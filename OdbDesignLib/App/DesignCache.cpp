#include "DesignCache.h"
#include "ArchiveExtractor.h"
#include "Logger.h"
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include "../FileModel/Design/FileArchive.h"
#include <memory>
#include "../ProductModel/Design.h"
#include <iosfwd>
#include <string>
#include <type_traits>
#include <StringVector.h>
#include <shared_mutex>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::App
{
    DesignCache::DesignCache(std::string directory) :
        m_directory(std::move(directory))
    {
		ensureDirectoryExists();
    }

    DesignCache::~DesignCache()
    {
        Clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::GetDesign(const std::string& designName)
    {
        return GetOrLoadSingleFlight<ProductModel::Design>(
            designName,
            m_designsByName,
            m_designsInFlight,
            [this, &designName]() { return LoadDesign(designName); },
            true);
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::GetFileArchive(const std::string& designName)
    {
        return GetFileArchiveInternal(designName, true);
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::GetFileArchiveInternal(const std::string& designName, bool updateState)
    {
        std::stringstream ss;
        ss << "Retrieving design \"" << designName << "\" from cache... ";
        logdebug(ss.str());

        return GetOrLoadSingleFlight<FileModel::Design::FileArchive>(
            designName,
            m_fileArchivesByName,
            m_fileArchivesInFlight,
            [this, &designName]() { return LoadFileArchive(designName); },
            updateState);
    }

    void DesignCache::AddFileArchive(const std::string& designName, std::shared_ptr<FileModel::Design::FileArchive> fileArchive, bool save)
    {
        {
            std::unique_lock<std::shared_mutex> writeLock(m_cacheMutex);
            m_fileArchivesByName[designName] = fileArchive;
        }

        // Charge the injected archive to the byte budget (never its own eviction
        // victim) and reflect the externally-completed load in the state machine.
        InsertLruAndEvict(designName);
        TransitionLoadState(designName, LoadState::Loaded);

        if (save)
        {
            // SaveFileArchive calls GetFileArchive internally, which takes the locks.
            // We must release before calling to avoid deadlock. (No locks held here.)
            SaveFileArchive(designName);
        }
    }

    bool DesignCache::SaveFileArchive(const std::string& designName)
    {
        auto fileArchive = GetFileArchive(designName);
        if (fileArchive != nullptr)
        {
            std::string directory;
            {
                std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
                directory = m_directory;
            }
            return fileArchive->SaveFileModel(directory);
        }
        return false;
    }

    std::vector<std::string> DesignCache::getLoadedDesignNames(const std::string& filter) const
    {
        std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
        std::vector<std::string> loadedDesigns;
        for (const auto& kv : m_designsByName)
        {
            loadedDesigns.push_back(kv.first);
        }
        return loadedDesigns;
    }

    std::vector<std::string> DesignCache::getLoadedFileArchiveNames(const std::string& filter) const
    {
        std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
        std::vector<std::string> loadedFileArchives;
        for (const auto& kv : m_fileArchivesByName)
		{
			loadedFileArchives.push_back(kv.first);
		}
        return loadedFileArchives;
    }

    std::vector<std::string> DesignCache::getUnloadedDesignNames(const std::string& filter) const
    {
        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        std::vector<std::string> unloadedNames;

        //try
        {
            path dir(directory);
            for (const auto& entry : directory_iterator(dir))
            {
                if (entry.is_regular_file())
                {
                    unloadedNames.push_back(entry.path().stem().string());
                }
            }
        }
        //catch (std::filesystem::filesystem_error& fe)
        //{
        //    logexception(fe);
        //    // re-throw it so we get a HTTP 500 response to the client
        //    throw fe;
        //}

        return unloadedNames;
    }

    int DesignCache::loadAllFileArchives(bool stopOnError)
    {
        int loaded = 0;

        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (ArchiveExtractor::IsArchiveTypeSupported(entry.path().filename()))
                {
                    try
                    {
                        auto pFileArchive = GetFileArchive(entry.path().stem().string());
                        if (pFileArchive != nullptr)
                        {
                            loaded++;
                        }
                    }
                    catch (std::exception& e)
                    {
                        // continue if we encounter an error loading one
                        logexception(e);
                        if (stopOnError) throw e;
                    }
                }
            }
       }

        return loaded;
    }

    int DesignCache::loadAllDesigns(bool stopOnError)
    {
        int loaded = 0;

        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (ArchiveExtractor::IsArchiveTypeSupported(entry.path().filename()))
                {
                    try
                    {
                        auto pDesign = GetDesign(entry.path().stem().string());
                        if (pDesign != nullptr)
                        {
                            loaded++;
                        }
                    }
                    catch (std::exception& e)
                    {
                        logexception(e);
                        if (stopOnError)
                        {
                            throw;
                        }
                    }
                }
            }
        }

        return loaded;
    }

    int DesignCache::loadFileArchives(const StringVector& names)
    {
        int loaded = 0;

        for (const auto& name : names)
        {
            try
            {
                auto pFileArchive = GetFileArchive(name);
                if (pFileArchive != nullptr)
                {
                    loaded++;
                }
            }
            catch (std::exception& e)
            {
                // continue on error
                logexception(e);
            }
        }

        return loaded;
    }

    int DesignCache::loadDesigns(const StringVector& names)
    {
        int loaded = 0;

        for (const auto& name : names)
        {
            try
            {
                auto pDesign = GetDesign(name);
                if (pDesign != nullptr)
                {
                    loaded++;
                }
            }
            catch (std::exception& e)
            {
                // continue on error
                logexception(e);
            }
        }

        return loaded;
    }

    void DesignCache::setDirectory(const std::string& directory)
    {
        std::unique_lock<std::shared_mutex> writeLock(m_cacheMutex);
        m_directory = directory;
    }

    std::string DesignCache::getDirectory() const
    {
        std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
        return m_directory;
    }

    void DesignCache::Clear()
    {
        {
            std::unique_lock<std::shared_mutex> writeLock(m_cacheMutex);
            m_fileArchivesByName.clear();
            m_designsByName.clear();
        }
        {
            std::lock_guard<std::mutex> lruLock(m_lruMutex);
            m_lruEntries.clear();
            m_cachedBytes = 0;
        }
        {
            // Wipe the state map and bump the cache generation under the state
            // lock. Loads that registered before this Clear() see the epoch change
            // on completion and abandon their bookkeeping — no cache insert, no
            // LRU charge, no stale state transition — instead of re-populating
            // the just-wiped cache with a phantom entry. Their in-flight futures
            // are left to their owning threads (which erase their own entries);
            // parsed values are still delivered to the direct callers holding
            // those futures.
            std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
            m_loadStatesByName.clear();
            ++m_epoch;
        }
    }

    DesignCache::LoadState DesignCache::GetLoadState(const std::string& designName) const
    {
        std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
        auto stateIt = m_loadStatesByName.find(designName);
        if (stateIt != m_loadStatesByName.end())
        {
            return stateIt->second;
        }
        return LoadState::Unloaded;
    }

    void DesignCache::AddLoadObserver(LoadEventCallback callback)
    {
        std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
        m_loadObservers.push_back(std::move(callback));
    }

    // Epoch snapshot comparison for Clear()-during-load detection; see m_epoch.
    bool DesignCache::WasClearedSince(std::uint64_t epoch) const
    {
        std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
        return m_epoch != epoch;
    }

    void DesignCache::setCacheMaxBytes(std::uint64_t maxBytes)
    {
        std::vector<std::string> evicted;
        {
            std::lock_guard<std::mutex> lruLock(m_lruMutex);
            m_maxBytes = maxBytes;
            // No protected name: shrinking the budget evicts immediately
            evicted = EvictOverBudgetLocked("");
        }
        EvictCacheEntries(evicted);
    }

    std::uint64_t DesignCache::getCacheMaxBytes() const
    {
        std::lock_guard<std::mutex> lruLock(m_lruMutex);
        return m_maxBytes;
    }

    void DesignCache::ensureDirectoryExists() const
    {
        if (!std::filesystem::exists(m_directory))
        {
            // create directory
            try
            {
                std::filesystem::create_directories(m_directory);
            }
            catch (const std::exception& e)
            {
                std::string msg = "Failed to create design cache directory: " + m_directory;
                logexception_msg(e, msg);
                throw e;
			}
        }
    }

    std::shared_ptr<ProductModel::Design> DesignCache::LoadDesign(const std::string& designName)
    {
        // NOTE: This is a lock-free I/O helper. It does NOT modify the cache maps.
        // Cache insertion is handled by GetOrLoadSingleFlight().
        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().stem() == designName)
                {
                    // GetFileArchive is thread-safe, manages its own locking.
                    // updateState=false: the archive is a sub-step of this design load
                    // and must not flip the design's load state mid-load.
                    auto pFileModel = GetFileArchiveInternal(designName, false);
                    if (pFileModel != nullptr)
                    {
                        auto pDesign = std::make_shared<ProductModel::Design>();
                        if (pDesign->Build(pFileModel))
                        {
                            return pDesign;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        return nullptr;
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::LoadFileArchive(const std::string& designName)
    {
        // NOTE: This is a lock-free I/O helper. It does NOT modify the cache maps.
        // Cache insertion is handled by GetOrLoadSingleFlight().
        auto fileFound = false;

        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        // skip inaccessible files and do not follow symlinks
        const auto options = directory_options::skip_permission_denied;
        for (const auto& entry : directory_iterator(directory, options))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().stem() == designName)
                {
                    fileFound = true;

                    loginfo("file found: [" + entry.path().string() + "], attempting to parse...");

                    auto pFileArchive = std::make_shared<FileModel::Design::FileArchive>(entry.path().string());
                    if (pFileArchive->ParseFileModel())
                    {
                        return pFileArchive;
                    }

                    // A matching file was found but failed to extract or parse. Treat this
                    // as a corrupt/unloadable design (error) rather than a missing one, so
                    // callers surface INTERNAL/500 instead of a misleading NOT_FOUND/404.
                    // (ParseFileModel already throws on parse failure; this covers the
                    // extraction / missing-root-dir paths that return false.)
                    // The file path is intentionally omitted here to avoid leaking server
                    // filesystem details to API clients (it is logged above for diagnostics).
                    throw std::runtime_error(
                        "Failed to load design \"" + designName + "\": could not extract or parse archive");
                }
            }
        }

        if (!fileFound)
        {
            logwarn("Failed to find file for design \"" + designName + "\"");

            logdebug("Listing all files in directory: " + directory);
            for (const auto& entry : directory_iterator(directory, options))
            {
                if (entry.is_regular_file())
                {
                    logdebug("Found file: " + entry.path().filename().string() + " (stem: " + entry.path().stem().string() + ")");
                }
            }
        }

        return nullptr;
    }

    void DesignCache::TouchLru(const std::string& designName)
    {
        std::lock_guard<std::mutex> lruLock(m_lruMutex);
        auto findIt = m_lruEntries.find(designName);
        if (findIt != m_lruEntries.end())
        {
            findIt->second.lastServed = std::chrono::steady_clock::now();
        }
        // No entry: the name raced with eviction, was never charged, or belonged
        // to a load abandoned across Clear(). Touch is a no-op for absent names —
        // it never inserts — so a joiner of an abandoned load cannot phantom-charge
        // the byte budget for an uncached name.
    }

    void DesignCache::InsertLruAndEvict(const std::string& designName)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto bytes = EstimateDesignBytes(designName);

        std::vector<std::string> evicted;
        {
            std::lock_guard<std::mutex> lruLock(m_lruMutex);
            auto findIt = m_lruEntries.find(designName);
            if (findIt != m_lruEntries.end())
            {
                findIt->second.lastServed = now;
            }
            else
            {
                m_lruEntries.emplace(designName, LruEntry{bytes, now});
                m_cachedBytes += bytes;
            }
            evicted = EvictOverBudgetLocked(designName);
        }
        EvictCacheEntries(evicted);
    }

    // Requires m_lruMutex held. May acquire m_loadStateMutex (the one permitted
    // nesting) to skip in-flight (Loading) designs. Returns evicted names; cache-map
    // and state cleanup happens in EvictCacheEntries after m_lruMutex is released.
    std::vector<std::string> DesignCache::EvictOverBudgetLocked(const std::string& protectedName)
    {
        std::vector<std::string> evicted;

        if (m_maxBytes == 0)
        {
            // Budget of 0 disables eviction
            return evicted;
        }

        while (m_cachedBytes > m_maxBytes)
        {
            auto victimIt = m_lruEntries.end();
            auto oldest = std::chrono::steady_clock::time_point::max();

            for (auto it = m_lruEntries.begin(); it != m_lruEntries.end(); ++it)
            {
                if (it->first == protectedName) continue;       // never evict the entry being served
                if (it->second.lastServed >= oldest) continue;  // least-recently-served first
                {
                    std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
                    auto stateIt = m_loadStatesByName.find(it->first);
                    if (stateIt != m_loadStatesByName.end() && stateIt->second == LoadState::Loading)
                    {
                        continue;  // never evict an in-flight design
                    }
                }
                oldest = it->second.lastServed;
                victimIt = it;
            }

            if (victimIt == m_lruEntries.end())
            {
                // Nothing evictable (only the protected/in-flight entries remain);
                // over-budget is tolerated rather than dropping protected work.
                break;
            }

            m_cachedBytes -= victimIt->second.estimatedBytes;
            const auto evictedName = victimIt->first;
            m_lruEntries.erase(victimIt);
            evicted.push_back(evictedName);
        }

        return evicted;
    }

    void DesignCache::EvictCacheEntries(const std::vector<std::string>& names)
    {
        for (const auto& name : names)
        {
            {
                std::unique_lock<std::shared_mutex> writeLock(m_cacheMutex);
                m_designsByName.erase(name);
                m_fileArchivesByName.erase(name);
            }

            loginfo("Evicted design \"" + name + "\" from cache (over byte budget)");

            TransitionLoadState(name, LoadState::Unloaded);
        }
    }

    std::uint64_t DesignCache::EstimateDesignBytes(const std::string& designName) const
    {
        // Simple estimate: archive file size on disk + a small constant. Serialized
        // response sizes are added to this estimate in M1.4 (response cache).
        std::string directory;
        {
            std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
            directory = m_directory;
        }

        std::error_code ec;
        const auto options = directory_options::skip_permission_denied;
        directory_iterator dirIt(directory, options, ec);
        if (ec)
        {
            return DESIGN_BYTES_OVERHEAD;
        }

        for (const auto& entry : dirIt)
        {
            if (entry.is_regular_file() && entry.path().stem() == designName)
            {
                const auto size = entry.file_size(ec);
                if (ec)
                {
                    return DESIGN_BYTES_OVERHEAD;
                }
                return static_cast<std::uint64_t>(size) + DESIGN_BYTES_OVERHEAD;
            }
        }

        return DESIGN_BYTES_OVERHEAD;
    }

    void DesignCache::TransitionLoadState(const std::string& designName, LoadState to)
    {
        LoadObserverList observers;
        auto from = LoadState::Unloaded;
        {
            std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
            auto stateIt = m_loadStatesByName.find(designName);
            from = (stateIt != m_loadStatesByName.end()) ? stateIt->second : LoadState::Unloaded;
            if (from == to)
            {
                // Not a transition (e.g. an archive load under an already-Loading
                // design, or a duplicate completion) — no observer event.
                return;
            }
            m_loadStatesByName[designName] = to;
            observers = m_loadObservers;
        }
        NotifyLoadObservers(designName, from, to, observers);
    }

    // Removes the design's state entry and reports the revert to Unloaded. If no
    // entry exists (nothing was recorded, or Clear() already wiped it) this is a
    // no-op: the name is already at the implicit Unloaded, so no event is due.
    void DesignCache::EraseLoadState(const std::string& designName)
    {
        LoadObserverList observers;
        auto from = LoadState::Unloaded;
        {
            std::lock_guard<std::mutex> stateLock(m_loadStateMutex);
            auto stateIt = m_loadStatesByName.find(designName);
            if (stateIt == m_loadStatesByName.end())
            {
                return;
            }
            from = stateIt->second;
            m_loadStatesByName.erase(stateIt);
            observers = m_loadObservers;
        }
        if (from != LoadState::Unloaded)
        {
            NotifyLoadObservers(designName, from, LoadState::Unloaded, observers);
        }
    }

    // Observers run with no locks held (they may re-enter the cache) and must be
    // cheap/non-blocking; a throwing observer is logged and skipped.
    void DesignCache::NotifyLoadObservers(const std::string& designName, LoadState from, LoadState to, const LoadObserverList& observers)
    {
        for (const auto& observer : observers)
        {
            try
            {
                observer(designName, from, to);
            }
            catch (const std::exception& e)
            {
                logwarn(std::string("Design load observer threw: ") + e.what());
            }
            catch (...)
            {
                logwarn("Design load observer threw an unknown exception");
            }
        }
    }
}
