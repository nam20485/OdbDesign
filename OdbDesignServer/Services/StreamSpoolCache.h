#pragma once

// StreamSpoolCache — disk-backed spool of a design's GetDesignStream chunk
// sequence (M6.3, getdesign-streaming-oom-plan.md).
//
// The monolithic in-RAM response cache (DesignCache M1.4) is unusable for
// over-budget designs: its payload IS the multi-GB serialized Design. For
// those designs the first GetDesignStream fill tees every chunk to a spool
// file while writing it to the wire; subsequent (warm) requests re-emit the
// chunks by reading the file record-by-record — never materializing more
// than one chunk of heap. The RAM cost of a warm hit is page cache, which
// the kernel reclaims under pressure, instead of anonymous heap.
//
// Spool file format: length-delimited protobuf records — each record is the
// standard varint-encoded byte length of the serialized Odb::Grpc::DesignChunk
// followed by those bytes, in chunk emission order, so a spool replays the
// exact chunk sequence (and therefore the exact monolithic Design bytes after
// reassembly).
//
// Identity/invalidation: an entry is valid for (design name, response-cache
// generation). DesignCache bumps the generation on AddFileArchive (re-upload),
// InvalidateDesign, and LRU eviction, so a stale spool is detected on the next
// open and the next fill re-spools the fresh contents. The cache owns a
// private spool directory (per service instance, under the system temp dir by
// default, removed in the destructor) so parallel server/test instances never
// touch each other's files; a total-bytes cap LRU-evicts spool files beyond it.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>

#include <google/protobuf/message.h>

namespace OdbDesignServer
{
    namespace Services
    {
        class StreamSpoolCache
        {
        public:
            // Creates a cache with a private spool directory under the system
            // temp directory (keeps parallel instances isolated; the directory
            // is removed in the destructor).
            StreamSpoolCache();

            // Creates a cache rooted at an explicit directory. The directory is
            // created if missing; its spool files are NOT removed wholesale by
            // the destructor (only files this cache itself published).
            StreamSpoolCache(const std::filesystem::path& directory, std::uint64_t maxTotalBytes);
            ~StreamSpoolCache();

            StreamSpoolCache(const StreamSpoolCache&) = delete;
            StreamSpoolCache& operator=(const StreamSpoolCache&) = delete;

            // Sequential record reader over an open spool file. Never holds
            // more than one record's bytes in heap.
            class Reader
            {
            public:
                // Reads the next length-delimited record into `record`.
                // Returns false at end-of-spool or on a read/parse failure;
                // Failed() distinguishes the two. A failure poisons the reader.
                bool Next(google::protobuf::Message& record);
                bool Failed() const { return m_failed; }
                std::uint64_t RecordsRead() const { return m_recordsRead; }

            private:
                friend class StreamSpoolCache;
                std::ifstream m_stream;
                std::uint64_t m_recordsRead = 0;
                bool m_failed = false;
            };

            // Append-only record writer over a spool file. Commit() publishes
            // the file for warm serving; Abort() deletes it. An uncommitted
            // writer aborts in its destructor.
            class Writer
            {
            public:
                Writer() = default;
                ~Writer();
                Writer(Writer&& other) noexcept;
                Writer& operator=(Writer&& other) noexcept;

                // Appends one length-delimited record. Returns false on I/O or
                // serialization failure (the writer is poisoned afterwards).
                bool Append(const google::protobuf::Message& record);

                // Flushes, closes, and publishes the spool entry.
                void Commit();
                // Closes and deletes the spool file without publishing.
                void Abort();

                std::uint64_t BytesWritten() const { return m_bytesWritten; }
                std::uint64_t RecordsWritten() const { return m_recordsWritten; }
                bool OpenForWrite() const { return m_openForWrite; }

            private:
                friend class StreamSpoolCache;
                void Close(bool commit);

                std::ofstream m_stream;
                std::filesystem::path m_path;
                StreamSpoolCache* m_pCache = nullptr;
                std::string m_designName;
                std::uint64_t m_generation = 0;
                std::uint64_t m_bytesWritten = 0;
                std::uint64_t m_recordsWritten = 0;
                bool m_openForWrite = false;
                bool m_committed = false;
            };

            // True (and fills `readerOut`) when a spool valid for (name,
            // generation) exists and opens. A stale entry is dropped (its file
            // deleted). False when no valid spool exists: cold fill required.
            bool TryOpenReader(const std::string& designName, std::uint64_t generation, Reader& readerOut);

            // Opens a fresh spool file for (name, generation). Returns false
            // when the spool directory is unusable (caller serves the stream
            // without spooling).
            bool BeginWrite(const std::string& designName, std::uint64_t generation, Writer& writerOut);

            // Drops the design's spool entry (and file), if any.
            void Drop(const std::string& designName);

            // Counters (observability/tests).
            std::uint64_t WarmOpens() const;
            std::uint64_t TotalSpoolBytes() const;
            std::size_t EntryCount() const;

        private:
            struct Entry
            {
                std::filesystem::path path;
                std::uint64_t generation = 0;
                std::uint64_t bytes = 0;
                std::uint64_t lastUsed = 0;
            };

            void InitializeDirectory(const std::filesystem::path& directory, bool ownsDirectory, std::uint64_t maxTotalBytes);
            std::filesystem::path MakeSpoolPath(const std::string& designName, std::uint64_t generation, std::uint64_t fillId) const;
            void PublishEntry(const std::string& designName, const Entry& entry);
            void EvictOverBudgetLocked(const std::string& protectedName);

            mutable std::mutex m_mutex;
            std::filesystem::path m_directory;
            bool m_ownsDirectory = false;
            std::uint64_t m_maxTotalBytes = 0;
            std::uint64_t m_totalBytes = 0;
            std::uint64_t m_warmOpens = 0;
            std::uint64_t m_useCounter = 0;
            std::uint64_t m_fillCounter = 0;
            std::map<std::string, Entry> m_entriesByName;
            bool m_usable = false;
        };
    }
}
