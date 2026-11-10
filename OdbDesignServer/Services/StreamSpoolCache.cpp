#include "StreamSpoolCache.h"
#include <Logger.h>

#include <atomic>
#include <chrono>
#include <system_error>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace OdbDesignServer
{
    namespace Services
    {
        namespace
        {
            // 64-bit FNV-1a: collision-free-enough namespacing of spool file
            // names (not a security boundary — the design name may contain
            // filesystem-hostile characters).
            std::uint64_t fnv1a64(const std::string& s)
            {
                std::uint64_t hash = 14695981039346656037ull;
                for (const char c : s)
                {
                    hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
                    hash *= 1099511628211ull;
                }
                return hash;
            }

            // Filesystem-safe projection of the design name (kept short; the
            // FNV suffix disambiguates).
            std::string sanitize_name(const std::string& designName)
            {
                std::string out;
                out.reserve(designName.size() < 48 ? designName.size() : 48);
                for (const char c : designName)
                {
                    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
                    out.push_back(ok ? c : '_');
                    if (out.size() >= 48) break;
                }
                return out;
            }

            unsigned long current_pid()
            {
#if defined(_WIN32)
                return static_cast<unsigned long>(_getpid());
#else
                return static_cast<unsigned long>(::getpid());
#endif
            }

            void append_varint(std::string& out, std::uint64_t value)
            {
                while (value >= 0x80)
                {
                    out.push_back(static_cast<char>((value & 0x7f) | 0x80));
                    value >>= 7;
                }
                out.push_back(static_cast<char>(value));
            }

            enum class VarintResult
            {
                Ok,
                CleanEof,   // no bytes consumed: legitimate end of the record stream
                Error       // truncated varint or malformed (too long) encoding
            };

            VarintResult read_varint(std::istream& in, std::uint64_t& valueOut)
            {
                std::uint64_t value = 0;
                int shift = 0;
                for (int i = 0; i < 10; ++i)
                {
                    const int byte = in.get();
                    if (byte == std::char_traits<char>::eof())
                    {
                        return (i == 0) ? VarintResult::CleanEof : VarintResult::Error;
                    }
                    value |= static_cast<std::uint64_t>(byte & 0x7f) << shift;
                    if ((byte & 0x80) == 0)
                    {
                        valueOut = value;
                        return VarintResult::Ok;
                    }
                    shift += 7;
                }
                return VarintResult::Error;
            }

            // Sanity cap for one record: chunk payloads are bounded by batch
            // sizes (single-digit MBs); refuse absurd lengths from a corrupt
            // file instead of allocating them.
            constexpr std::uint64_t kMaxRecordBytes = 512ull * 1024ull * 1024ull;

            // Process-global fill counter: the spool path embeds pid + fillId
            // so concurrent fills never share a file. pid uniquifies across
            // processes; the GLOBAL counter (not a per-instance member)
            // uniquifies same-process cache instances sharing an explicit
            // spool directory — two instances would otherwise both start at
            // fill 1 and truncate-open each other's path.
            std::atomic<std::uint64_t> g_fillCounter{ 0 };
        }

        // ---- Reader ----

        bool StreamSpoolCache::Reader::Next(google::protobuf::Message& record)
        {
            if (m_failed || !m_stream.is_open())
            {
                return false;
            }

            std::uint64_t length = 0;
            const auto varint = read_varint(m_stream, length);
            if (varint == VarintResult::CleanEof)
            {
                return false;
            }
            if (varint == VarintResult::Error || length == 0 || length > kMaxRecordBytes)
            {
                m_failed = true;
                return false;
            }

            std::string bytes(static_cast<std::size_t>(length), '\0');
            m_stream.read(bytes.data(), bytes.size());
            if (static_cast<std::uint64_t>(m_stream.gcount()) != length)
            {
                m_failed = true;
                return false;
            }
            if (!record.ParseFromString(bytes))
            {
                m_failed = true;
                return false;
            }
            ++m_recordsRead;
            return true;
        }

        // ---- Writer ----

        StreamSpoolCache::Writer::~Writer()
        {
            if (m_openForWrite && !m_committed)
            {
                Close(/*commit=*/false);
            }
        }

        StreamSpoolCache::Writer::Writer(Writer&& other) noexcept
            : m_stream(std::move(other.m_stream)),
              m_path(std::move(other.m_path)),
              m_pCache(other.m_pCache),
              m_designName(std::move(other.m_designName)),
              m_generation(other.m_generation),
              m_bytesWritten(other.m_bytesWritten),
              m_recordsWritten(other.m_recordsWritten),
              m_openForWrite(other.m_openForWrite),
              m_committed(other.m_committed),
              m_publishSucceeded(other.m_publishSucceeded)
        {
            other.m_pCache = nullptr;
            other.m_openForWrite = false;
            other.m_committed = false;
            other.m_publishSucceeded = false;
        }

        StreamSpoolCache::Writer& StreamSpoolCache::Writer::operator=(Writer&& other) noexcept
        {
            if (this != &other)
            {
                if (m_openForWrite && !m_committed)
                {
                    Close(/*commit=*/false);
                }
                m_stream = std::move(other.m_stream);
                m_path = std::move(other.m_path);
                m_pCache = other.m_pCache;
                m_designName = std::move(other.m_designName);
                m_generation = other.m_generation;
                m_bytesWritten = other.m_bytesWritten;
                m_recordsWritten = other.m_recordsWritten;
                m_openForWrite = other.m_openForWrite;
                m_committed = other.m_committed;
                m_publishSucceeded = other.m_publishSucceeded;
                other.m_pCache = nullptr;
                other.m_openForWrite = false;
                other.m_committed = false;
                other.m_publishSucceeded = false;
            }
            return *this;
        }

        bool StreamSpoolCache::Writer::Append(const google::protobuf::Message& record)
        {
            if (!m_openForWrite || !m_stream.is_open())
            {
                return false;
            }

            std::string bytes;
            if (!record.SerializeToString(&bytes))
            {
                return false;
            }

            std::string framed;
            framed.reserve(bytes.size() + 10);
            append_varint(framed, bytes.size());
            framed.append(bytes);

            m_stream.write(framed.data(), framed.size());
            if (!m_stream.good())
            {
                return false;
            }
            m_bytesWritten += framed.size();
            ++m_recordsWritten;
            return true;
        }

        bool StreamSpoolCache::Writer::Commit()
        {
            if (!m_openForWrite || m_committed)
            {
                return false;
            }
            Close(/*commit=*/true);
            return m_publishSucceeded;
        }

        void StreamSpoolCache::Writer::Abort()
        {
            if (!m_openForWrite || m_committed)
            {
                return;
            }
            Close(/*commit=*/false);
        }

        void StreamSpoolCache::Writer::Close(bool commit)
        {
            m_openForWrite = false;
            bool streamOk = true;
            if (m_stream.is_open())
            {
                // ofstream buffers; an I/O error such as ENOSPC typically
                // surfaces only at flush/close time. A failed flush means the
                // final bytes never reached the disk — publishing the file
                // would poison the warm path with a silently truncated spool.
                m_stream.flush();
                streamOk = m_stream.good();
                m_stream.close();
            }

            if (commit)
            {
                m_committed = true;
                m_publishSucceeded = streamOk && m_pCache != nullptr;
                if (m_publishSucceeded)
                {
                    m_publishSucceeded = m_pCache->PublishEntry(m_designName,
                        Entry{ m_path, m_generation, m_bytesWritten, m_recordsWritten, 0 });
                }
                else
                {
                    std::error_code ec;
                    std::filesystem::remove(m_path, ec); // best-effort
                    logerror("StreamSpoolCache: spool flush/close failed for \"" + m_designName +
                        "\"; incomplete spool deleted, not published (next request refills cold)");
                }
            }
            else
            {
                std::error_code ec;
                std::filesystem::remove(m_path, ec); // best-effort temp cleanup
            }
        }

        // ---- Cache ----

        StreamSpoolCache::StreamSpoolCache(std::uint64_t maxTotalBytes)
        {
            namespace fs = std::filesystem;

            std::error_code ec;
            const auto base = fs::temp_directory_path(ec);
            if (ec)
            {
                logerror("StreamSpoolCache: cannot resolve temp directory; spooling disabled");
                return;
            }

            // Unique per-instance directory (pid + steady ticks);
            // create_directory fails on collision, so retry with a fresh suffix.
            const auto pid = current_pid();
            for (int attempt = 0; attempt < 8; ++attempt)
            {
                const auto tick = static_cast<std::uint64_t>(
                    std::chrono::steady_clock::now().time_since_epoch().count()) + attempt;
                const auto candidate = base / ("odbdesign-spool-" + std::to_string(pid) + "-" + std::to_string(tick));
                std::error_code createEc;
                if (fs::create_directory(candidate, createEc) && !createEc)
                {
                    InitializeDirectory(candidate, /*ownsDirectory=*/true, maxTotalBytes);
                    return;
                }
            }
            logerror("StreamSpoolCache: failed to create spool directory; spooling disabled");
        }

        StreamSpoolCache::StreamSpoolCache(const std::filesystem::path& directory, std::uint64_t maxTotalBytes)
        {
            InitializeDirectory(directory, /*ownsDirectory=*/false, maxTotalBytes);
        }

        StreamSpoolCache::~StreamSpoolCache()
        {
            if (m_ownsDirectory)
            {
                std::error_code ec;
                std::filesystem::remove_all(m_directory, ec); // best-effort
                return;
            }
            // Explicit (caller-owned) directory: still remove the spool files
            // THIS instance published — the constructor's documented contract.
            // Without it a clean shutdown leaves every entry behind, and a
            // restarted service neither indexes nor accounts for them, so
            // restarts accumulate multi-GB spools outside the configured cap.
            // Unrelated files are untouched; POSIX unlink-while-open keeps
            // any concurrent warm reader's fd valid through the removal.
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& kv : m_entriesByName)
            {
                std::error_code ec;
                std::filesystem::remove(kv.second.path, ec); // best-effort
            }
        }

        void StreamSpoolCache::InitializeDirectory(const std::filesystem::path& directory, bool ownsDirectory, std::uint64_t maxTotalBytes)
        {
            namespace fs = std::filesystem;
            std::error_code ec;
            fs::create_directories(directory, ec);
            if (ec || !fs::is_directory(directory))
            {
                logerror("StreamSpoolCache: spool directory unusable (" + directory.string() + "); spooling disabled");
                return;
            }
            m_directory = directory;
            m_ownsDirectory = ownsDirectory;
            m_maxTotalBytes = maxTotalBytes;
            m_usable = true;
        }

        std::filesystem::path StreamSpoolCache::MakeSpoolPath(const std::string& designName, std::uint64_t generation, std::uint64_t fillId) const
        {
            auto name = sanitize_name(designName);
            if (name.empty())
            {
                name = "design";
            }
            // pid uniquifies across processes sharing an explicit spool
            // directory; fillId uniquifies concurrent fills within one
            // process. Without the per-fill suffix two cold fills of the
            // same (design, generation) would open one path with
            // std::ios::trunc and interleave their length-delimited writes.
            return m_directory / (name + "-" + std::to_string(fnv1a64(designName)) + "-" +
                std::to_string(generation) + "-" +
                std::to_string(current_pid()) + "-" + std::to_string(fillId) + ".spool");
        }

        bool StreamSpoolCache::TryOpenReader(const std::string& designName, std::uint64_t generation, Reader& readerOut)
        {
            if (!m_usable)
            {
                return false;
            }

            std::filesystem::path path;
            std::uint64_t expectedRecords = 0;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto findIt = m_entriesByName.find(designName);
                if (findIt == m_entriesByName.end())
                {
                    return false;
                }
                if (findIt->second.generation != generation)
                {
                    // Stale spool (the design was invalidated/evicted/re-uploaded
                    // since it was written): drop it and refill cold.
                    std::error_code ec;
                    std::filesystem::remove(findIt->second.path, ec);
                    m_totalBytes -= findIt->second.bytes;
                    m_entriesByName.erase(findIt);
                    return false;
                }
                path = findIt->second.path;
                expectedRecords = findIt->second.records;
                findIt->second.lastUsed = ++m_useCounter;
            }

            std::error_code ec;
            if (!std::filesystem::is_regular_file(path, ec))
            {
                // Path-guarded: a concurrent PublishEntry (a different
                // generation, or a same-generation last-committer-wins fill)
                // may have swapped in a replacement and unlinked the old path
                // between the lock-guarded lookup above and this probe; only
                // the entry still referring to the file we observed may be
                // dropped.
                DropStaleEntry(designName, path);
                return false;
            }

            readerOut.m_stream.open(path, std::ios::binary);
            if (!readerOut.m_stream.is_open())
            {
                return false;
            }
            readerOut.m_recordsRead = 0;
            // Committed record count: a clean EOF short of it means the file
            // was truncated AT a record boundary (or emptied) — framing alone
            // cannot distinguish that from a legitimate end-of-spool, so the
            // reader carries the count and Truncated() reports the mismatch.
            readerOut.m_expectedRecords = expectedRecords;
            readerOut.m_failed = false;

            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_warmOpens;
            return true;
        }

        bool StreamSpoolCache::BeginWrite(const std::string& designName, std::uint64_t generation, Writer& writerOut)
        {
            if (!m_usable)
            {
                return false;
            }

            // Each fill writes its OWN file (the path embeds a per-fill
            // suffix — see MakeSpoolPath): concurrent cold fills of the same
            // (design, generation) must never share a file, because the
            // ios::trunc open would interleave their length-delimited writes
            // and publish a corrupt spool. Publishing is a bookkeeping
            // insert, not a rename: the last committer wins, and
            // PublishEntry removes the loser's (distinct) file. A warm
            // reader already streaming the loser's file is safe on POSIX
            // (unlink-while-open keeps its fd valid); on Windows the remove
            // can fail while the handle is open — best-effort, the orphan is
            // reclaimed when an owned spool directory is torn down.
            std::uint64_t fillId = 0;
            {
                // Process-global counter (not a per-instance member): two
                // cache instances in one process sharing an explicit spool
                // directory would otherwise both start at fill 1 and produce
                // the same pathname, trunc-opening each other's spool. The
                // pid component in the path already uniquifies across
                // processes.
                fillId = ++g_fillCounter;
            }
            const auto path = MakeSpoolPath(designName, generation, fillId);

            writerOut.m_stream.open(path, std::ios::binary | std::ios::trunc);
            if (!writerOut.m_stream.is_open())
            {
                return false;
            }
            writerOut.m_pCache = this;
            writerOut.m_path = path;
            writerOut.m_designName = designName;
            writerOut.m_generation = generation;
            writerOut.m_bytesWritten = 0;
            writerOut.m_recordsWritten = 0;
            writerOut.m_committed = false;
            writerOut.m_openForWrite = true;
            return true;
        }

        bool StreamSpoolCache::PublishEntry(const std::string& designName, const Entry& entry)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            Entry published = entry;
            published.lastUsed = ++m_useCounter;

            // A single entry larger than the whole budget would evict every
            // other design and still leave the cache over cap indefinitely
            // (the just-published design is eviction-protected). Reject it
            // instead: the completed stream simply stays unspooled and every
            // request for this design refills cold — raise spool_max_bytes
            // if that design should be warm-servable.
            if (m_maxTotalBytes != 0 && published.bytes > m_maxTotalBytes)
            {
                std::error_code ec;
                std::filesystem::remove(published.path, ec); // best-effort
                logwarn("StreamSpoolCache: spool for \"" + designName + "\" (" +
                    std::to_string(published.bytes) + " bytes) exceeds spool_max_bytes (" +
                    std::to_string(m_maxTotalBytes) + "); not published (design serves cold)");
                return false;
            }

            auto findIt = m_entriesByName.find(designName);
            if (findIt != m_entriesByName.end())
            {
                std::error_code ec;
                std::filesystem::remove(findIt->second.path, ec); // best-effort
                m_totalBytes -= findIt->second.bytes;
                findIt->second = published;
            }
            else
            {
                m_entriesByName.emplace(designName, published);
            }
            m_totalBytes += published.bytes;

            EvictOverBudgetLocked(designName);
            return true;
        }

        void StreamSpoolCache::Drop(const std::string& designName)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto findIt = m_entriesByName.find(designName);
            if (findIt == m_entriesByName.end())
            {
                return;
            }
            std::error_code ec;
            std::filesystem::remove(findIt->second.path, ec); // best-effort
            m_totalBytes -= findIt->second.bytes;
            m_entriesByName.erase(findIt);
        }

        void StreamSpoolCache::DropStaleEntry(const std::string& designName, const std::filesystem::path& expectedPath)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto findIt = m_entriesByName.find(designName);
            if (findIt == m_entriesByName.end() || findIt->second.path != expectedPath)
            {
                return; // replaced or removed since it was observed - leave the new entry alone
            }
            std::error_code ec;
            std::filesystem::remove(findIt->second.path, ec); // best-effort
            m_totalBytes -= findIt->second.bytes;
            m_entriesByName.erase(findIt);
        }

        void StreamSpoolCache::EvictOverBudgetLocked(const std::string& protectedName)
        {
            if (m_maxTotalBytes == 0)
            {
                return;
            }
            while (m_totalBytes > m_maxTotalBytes)
            {
                // Least-recently-used victim, never the just-published design.
                auto victimIt = m_entriesByName.end();
                for (auto it = m_entriesByName.begin(); it != m_entriesByName.end(); ++it)
                {
                    if (it->first == protectedName) continue;
                    if (victimIt == m_entriesByName.end() || it->second.lastUsed < victimIt->second.lastUsed)
                    {
                        victimIt = it;
                    }
                }
                if (victimIt == m_entriesByName.end())
                {
                    break; // only the protected entry remains; tolerate over-budget
                }
                std::error_code ec;
                std::filesystem::remove(victimIt->second.path, ec); // best-effort
                m_totalBytes -= victimIt->second.bytes;
                m_entriesByName.erase(victimIt);
            }
        }

        std::uint64_t StreamSpoolCache::WarmOpens() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_warmOpens;
        }

        std::uint64_t StreamSpoolCache::TotalSpoolBytes() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_totalBytes;
        }

        std::size_t StreamSpoolCache::EntryCount() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_entriesByName.size();
        }
    }
}
