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
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT DesignCache
	{
	public:
		// Lifecycle of a single design's load. Failed covers parse failure only
		// (a corrupt/unparseable archive); a design that is simply not found is a
		// miss, not a failure, and records no state entry (its name stays
		// Unloaded) so the state map cannot grow without bound on untrusted
		// names. Neither poisons the cache — the in-flight entry is erased so a
		// later request starts a fresh load.
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

		// Register a load-state observer. Observers are invoked synchronously on
		// the thread performing the transition (loader or evictor) with no cache
		// locks held; observers MUST be cheap and non-blocking.
		//
		// Re-entrancy contract: an observer MUST NOT request the design currently
		// being loaded on its own thread (GetDesign / GetFileArchive / anything
		// that reaches GetOrLoadSingleFlight for that name). The re-entrant call
		// would join the loader's own pending in-flight future and deadlock; such
		// re-entry is detected and rejected with std::runtime_error instead of
		// waiting. Requesting other designs is safe.
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

		// Cache generation, guarded by m_loadStateMutex: Clear() increments it. A
		// load captures it at in-flight registration; a mismatch at completion
		// means Clear() ran mid-parse and the load must abandon its bookkeeping
		// (no cache insert, no LRU charge, no state transition) while still
		// delivering the parsed value to its direct callers.
		std::uint64_t m_epoch = 0;

		// Protects the in-flight maps, m_loadStatesByName, m_loadObservers, and m_epoch
		mutable std::mutex m_loadStateMutex;

		// Thread-local re-entry guard: name of the design whose load the current
		// thread is running, set only while the Loading transition (and its
		// synchronous observers) is in progress on the loader thread. The joiner
		// path of GetOrLoadSingleFlight checks it and throws instead of joining
		// the thread's own pending future. Held in a function-local thread_local
		// because a static thread_local data member of an ODBDESIGN_EXPORT class
		// would give the variable dll interface, which MSVC rejects (C2492).
		static const std::string*& loadingDesignName()
		{
			thread_local const std::string* s_loadingDesignName = nullptr;
			return s_loadingDesignName;
		}

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
			std::uint64_t epochAtStart = 0;
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
					// Capture the cache generation at registration: a Clear() during
					// the parse bumps m_epoch and this load abandons its bookkeeping
					// on completion (see WasClearedSince below).
					epochAtStart = m_epoch;
				}
			}

			if (!isLoader)
			{
				// Observer re-entry guard: if this thread is itself the loader of
				// this design (an observer invoked during the Loading transition),
				// waiting here would join our own pending future and deadlock.
				// Reject the re-entry instead of blocking forever.
				const std::string* pLoadingName = loadingDesignName();
				if (pLoadingName != nullptr && *pLoadingName == designName)
				{
					throw std::runtime_error(
						"DesignCache observer re-entry: load observers must not call back into the DesignCache for the design being loaded");
				}

				// Another thread is parsing this design; wait on its result. A failed
				// parse rethrows here in the waiting thread.
				auto pValue = future.get();
				TouchLru(designName);
				return pValue;
			}

			if (updateState)
			{
				// Announce Loading with the re-entry marker set: observers run
				// synchronously on this (loader) thread, and one that re-enters the
				// cache for this design would otherwise join our own pending future
				// and deadlock (the marker makes such a call throw; see the joiner
				// path above).
				struct LoadingMarker
				{
					explicit LoadingMarker(const std::string& name) :
						m_previous(loadingDesignName())
					{
						loadingDesignName() = &name;
					}
					~LoadingMarker()
					{
						loadingDesignName() = m_previous;
					}
					const std::string* m_previous;
				} loadingMarker(designName);

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
				// record Failed, then drop the in-flight entry. The transition runs
				// while this thread still owns the in-flight entry: a caller arriving
				// mid-transition joins the already-failed future instead of
				// registering a fresh load whose Loading state our late Failed would
				// overwrite (state claiming Failed while a fresh load is in flight).
				promise.set_exception(std::current_exception());
				if (updateState && !WasClearedSince(epochAtStart))
				{
					TransitionLoadState(designName, LoadState::Failed);
				}
				eraseInFlight(inFlightMap, designName);
				throw;
			}

			if (pValue == nullptr)
			{
				// Design not found: nothing to cache. A missing design is not a
				// parse failure — erase the state entry the Loading announcement
				// recorded (name reverts to the implicit Unloaded) so
				// m_loadStatesByName stays bounded by names that map to real
				// archives instead of growing on every untrusted route parameter
				// ever requested. Parse exceptions from corrupt archives still
				// record Failed, bounded by the real design count. The erase runs
				// before the in-flight entry is dropped, mirroring the failure
				// path: a caller arriving mid-cleanup joins our already-fulfilled
				// future instead of registering a fresh load whose Loading our
				// late erase would clobber.
				promise.set_value(nullptr);
				if (updateState)
				{
					EraseLoadState(designName);
				}
				eraseInFlight(inFlightMap, designName);
				return nullptr;
			}

			if (WasClearedSince(epochAtStart))
			{
				// Clear() ran while we parsed: this generation of the cache is
				// gone. Abandon the bookkeeping — no cache insert, no LRU charge,
				// no state transition — so the just-wiped cache is not re-populated
				// with a phantom entry, but still deliver the parsed value to our
				// direct callers and drop the in-flight entry.
				promise.set_value(pValue);
				eraseInFlight(inFlightMap, designName);
				return pValue;
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

			// Record Loaded while the in-flight entry still exists: a caller
			// arriving mid-transition joins our future instead of starting a fresh
			// load whose Loading our Loaded would clobber, and joiners woken by
			// set_value below then observe Loaded.
			if (updateState)
			{
				TransitionLoadState(designName, LoadState::Loaded);
			}

			// Publish to joiners only once the cache entry and state are visible
			promise.set_value(pValue);
			eraseInFlight(inFlightMap, designName);
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

		// Removes a design's load-state entry entirely (its state becomes the
		// implicit Unloaded) and notifies observers of the transition. Used when
		// a load attempt ends without a result that should keep state (not-found):
		// erasing — rather than overwriting with Unloaded — is what bounds
		// m_loadStatesByName growth to names that map to real archives.
		void EraseLoadState(const std::string& designName);

		void NotifyLoadObservers(const std::string& designName, LoadState from, LoadState to, const LoadObserverList& observers);

		// True once Clear() has run after the given epoch snapshot was taken
		// (see m_epoch); a load in that situation abandons its bookkeeping.
		bool WasClearedSince(std::uint64_t epoch) const;

		constexpr inline static std::uint64_t DEFAULT_CACHE_MAX_BYTES = 4096ull * 1024ull * 1024ull;
		// Rough per-design overhead added to the archive file size; serialized response
		// sizes are added to this estimate in M1.4 (response cache).
		constexpr inline static std::uint64_t DESIGN_BYTES_OVERHEAD = 4096;

		constexpr inline static const char* DESIGN_EXTENSIONS[] = { "zip", "tgz", "tar.gz", "tar", "gzip" , "gz" };

	};
}
