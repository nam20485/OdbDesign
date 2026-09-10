#pragma once

#include "../FileModel/Design/FileArchive.h"
#include "../ProductModel/Design.h"
#include "../odbdesign_export.h"
#include "StringVector.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>


namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT DesignCache
	{
	public:
		// Lifecycle of a single design's load. Failed covers both parse failure and
		// design-not-found; neither poisons the cache — the in-flight entry is erased
		// so a later request starts a fresh load.
		enum class LoadState
		{
			Unloaded,
			Loading,
			Loaded,
			Failed
		};

		// Observer for load-state transitions. Invoked on the thread that performs the
		// transition (loader or evictor) with no cache locks held; observers MUST be
		// cheap and non-blocking.
		using LoadEventCallback = std::function<void(const std::string& designName, LoadState from, LoadState to)>;

		DesignCache(std::string directory);
		~DesignCache();

		std::shared_ptr<ProductModel::Design> GetDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchive(const std::string& designName);

		void AddFileArchive(const std::string& designName, std::shared_ptr<FileModel::Design::FileArchive> fileArchive, bool save);

		bool SaveFileArchive(const std::string& designName);

		std::vector<std::string> getLoadedDesignNames(const std::string& filter = "") const;
		std::vector<std::string> getLoadedFileArchiveNames(const std::string& filter = "") const;
		std::vector<std::string> getUnloadedDesignNames(const std::string& filter = "") const;

		int loadAllFileArchives(bool stopOnError);
		int loadAllDesigns(bool stopOnError);
		int loadFileArchives(const Utils::StringVector& names);
		int loadDesigns(const Utils::StringVector& names);

		void setDirectory(const std::string& directory);
		std::string getDirectory() const;

		void Clear();

		void ensureDirectoryExists() const;

		// ---- load state machine + observers (consumed by M1.2 RequestLoadDesign, M3.2 events) ----

		LoadState GetLoadState(const std::string& designName) const;
		void AddLoadObserver(LoadEventCallback callback);

		// Byte budget for LRU eviction; 0 disables eviction.
		void setCacheMaxBytes(std::uint64_t maxBytes);
		std::uint64_t getCacheMaxBytes() const;

	private:
		std::string m_directory;

		FileModel::Design::FileArchive::StringMap m_fileArchivesByName;
		ProductModel::Design::StringMap m_designsByName;

		// Protects m_fileArchivesByName, m_designsByName, and m_directory
		// Use shared_lock for reads, unique_lock for writes
		mutable std::shared_mutex m_cacheMutex;

		// ---- single-flight load bookkeeping ----
		//
		// Lock order (never acquired in reverse; only nesting allowed is
		// m_lruMutex -> m_loadStateMutex inside eviction):
		//   m_cacheMutex     -> (nothing)
		//   m_lruMutex       -> m_loadStateMutex
		//   m_loadStateMutex -> (nothing)
		// All other critical sections take a single lock at a time.

		using DesignFuture = std::shared_future<std::shared_ptr<ProductModel::Design>>;
		using FileArchiveFuture = std::shared_future<std::shared_ptr<FileModel::Design::FileArchive>>;
		using LoadStateMap = std::unordered_map<std::string, LoadState>;
		using LoadObserverList = std::vector<LoadEventCallback>;

		// Exactly one parse per design name: the thread that emplaces the entry runs
		// the parse and fulfils the promise; concurrent callers block on the same
		// shared_future. Erased on completion AND on failure (failed loads retry).
		std::unordered_map<std::string, DesignFuture> m_designsInFlight;
		std::unordered_map<std::string, FileArchiveFuture> m_fileArchivesInFlight;

		LoadStateMap m_loadStatesByName;
		LoadObserverList m_loadObservers;

		// Protects the in-flight maps, m_loadStatesByName, and m_loadObservers
		mutable std::mutex m_loadStateMutex;

		// ---- byte-budget LRU bookkeeping ----

		struct LruEntry
		{
			std::uint64_t estimatedBytes = 0;
			std::chrono::steady_clock::time_point lastServed;
		};

		using LruEntryMap = std::unordered_map<std::string, LruEntry>;

		// Keyed by design name: whether the archive, the Design, or both are cached,
		// a name carries a single LRU entry. m_cachedBytes is the sum of estimatedBytes.
		// Evicting a name removes its Design and FileArchive cache entries together;
		// a Design still referenced by a live caller keeps its shared_ptr to its
		// FileArchive, so in-use objects are never destroyed by eviction.
		LruEntryMap m_lruEntries;
		std::uint64_t m_cachedBytes = 0;
		std::uint64_t m_maxBytes = DEFAULT_CACHE_MAX_BYTES;

		// Protects m_lruEntries, m_cachedBytes, and m_maxBytes
		mutable std::mutex m_lruMutex;

		std::shared_ptr<ProductModel::Design> LoadDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> LoadFileArchive(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchiveInternal(const std::string& designName, bool updateState);

		// Core single-flight path behind GetDesign/GetFileArchive. Exactly one caller
		// per design name runs loadFn; joiners wait on the same shared_future (a failed
		// parse rethrows in each waiter). updateState=false for loads nested inside
		// another load: LoadDesign pulling the archive must not flip the design's load
		// state (Loading/Loaded) mid-load.
		template <typename T, typename LoadFn, typename CacheMap, typename InFlightMap>
		std::shared_ptr<T> GetOrLoadSingleFlight(
			const std::string& designName,
			CacheMap& cacheMap,
			InFlightMap& inFlightMap,
			LoadFn loadFn,
			bool updateState)
		{
			// Fast path: shared (read) lock for cache hit
			bool hit = false;
			std::shared_ptr<T> pCached;
			{
				std::shared_lock<std::shared_mutex> readLock(m_cacheMutex);
				auto findIt = cacheMap.find(designName);
				if (findIt != cacheMap.end())
				{
					pCached = findIt->second;
					hit = true;
				}
			}
			if (hit)
			{
				TouchLru(designName);
				return pCached;
			}

			// Join an in-flight load, or become its only runner
			std::promise<std::shared_ptr<T>> promise;
			std::shared_future<std::shared_ptr<T>> future = promise.get_future().share();
			bool isLoader = false;
			{
				std::lock_guard<std::mutex> lock(m_loadStateMutex);
				auto findIt = inFlightMap.find(designName);
				if (findIt != inFlightMap.end())
				{
					future = findIt->second;
				}
				else
				{
					inFlightMap.emplace(designName, future);
					isLoader = true;
				}
			}

			if (!isLoader)
			{
				// Another thread is parsing this design; wait on its result. A failed
				// parse rethrows here in the waiting thread.
				auto pValue = future.get();
				TouchLru(designName);
				return pValue;
			}

			if (updateState)
			{
				TransitionLoadState(designName, LoadState::Loading);
			}

			std::shared_ptr<T> pValue;
			try
			{
				pValue = loadFn();
			}
			catch (...)
			{
				// Fulfil the shared future first so joiners observe the failure, then
				// drop the in-flight entry so a later request starts a fresh load.
				promise.set_exception(std::current_exception());
				eraseInFlight(inFlightMap, designName);
				if (updateState)
				{
					TransitionLoadState(designName, LoadState::Failed);
				}
				throw;
			}

			if (pValue == nullptr)
			{
				// Design not found: nothing to cache. Record Failed (retryable) so
				// state consumers see the last attempt did not produce a design.
				promise.set_value(nullptr);
				eraseInFlight(inFlightMap, designName);
				if (updateState)
				{
					TransitionLoadState(designName, LoadState::Failed);
				}
				return nullptr;
			}

			// Insert into cache under exclusive lock (double-check another thread
			// or path — e.g. AddFileArchive — didn't insert while we parsed)
			{
				std::unique_lock<std::shared_mutex> writeLock(m_cacheMutex);
				auto findIt = cacheMap.find(designName);
				if (findIt != cacheMap.end())
				{
					pValue = findIt->second;
				}
				else
				{
					cacheMap[designName] = pValue;
				}
			}

			// Charge the byte budget and evict if over; this design is never its own victim
			InsertLruAndEvict(designName);

			// Publish to joiners only once the cache entry is visible
			promise.set_value(pValue);
			eraseInFlight(inFlightMap, designName);
			if (updateState)
			{
				TransitionLoadState(designName, LoadState::Loaded);
			}
			return pValue;
		}

		template <typename T>
		void eraseInFlight(std::unordered_map<std::string, std::shared_future<std::shared_ptr<T>>>& inFlightMap,
			const std::string& designName)
		{
			// The loader thread is the only eraser; erase-by-key is a no-op if Clear()
			// or a previous cycle removed the entry meanwhile.
			std::lock_guard<std::mutex> lock(m_loadStateMutex);
			inFlightMap.erase(designName);
		}

		void TouchLru(const std::string& designName);
		void InsertLruAndEvict(const std::string& designName);
		std::vector<std::string> EvictOverBudgetLocked(const std::string& protectedName);
		void EvictCacheEntries(const std::vector<std::string>& names);
		std::uint64_t EstimateDesignBytes(const std::string& designName) const;

		void TransitionLoadState(const std::string& designName, LoadState to);
		void NotifyLoadObservers(const std::string& designName, LoadState from, LoadState to, const LoadObserverList& observers);

		constexpr inline static std::uint64_t DEFAULT_CACHE_MAX_BYTES = 4096ull * 1024ull * 1024ull;
		// Rough per-design overhead added to the archive file size; serialized response
		// sizes are added to this estimate in M1.4 (response cache).
		constexpr inline static std::uint64_t DESIGN_BYTES_OVERHEAD = 4096;

		constexpr inline static const char* DESIGN_EXTENSIONS[] = { "zip", "tgz", "tar.gz", "tar", "gzip" , "gz" };

	};
}
