#pragma once

#include "../FileModel/Design/FileArchive.h"
#include "../ProductModel/Design.h"
#include "../odbdesign_export.h"
#include "StringVector.h"
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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

		// ---- background loads (consumed by M1.2 RequestLoadDesign + upload auto-warm) ----

		// Kicks a background load of the named design and returns immediately:
		// the parse runs on a detached thread through the same single-flight
		// path as a synchronous GetDesign, so state/observer transitions,
		// caching, and failure semantics (Failed is retryable) are identical.
		// Returns true when a fresh background load was started; false when one
		// is already pending or in flight for the name (the caller surfaces
		// that as "already loading"). Throws only if the load thread itself
		// could not be spawned; load failures are logged, never thrown here.
		bool LoadDesignAsync(const std::string& designName);

		// True when an archive file for the design name exists in the designs
		// directory (first regular file whose stem matches), without
		// triggering a load. Lets callers distinguish "no such archive"
		// (NOT_FOUND) from a parse failure (Failed load state).
		bool ContainsDesign(const std::string& designName) const;

		// Byte budget for LRU eviction; 0 disables eviction.
		void setCacheMaxBytes(std::uint64_t maxBytes);
		std::uint64_t getCacheMaxBytes() const;

		// Cap on concurrently parsing background loads; further loads queue on
		// the semaphore inside their background thread (LoadDesignAsync itself
		// never blocks). 0 = unbounded.
		void setMaxBackgroundLoads(std::size_t maxLoads);
		std::size_t getMaxBackgroundLoads() const;

		// ---- serialized-response cache (M1.4) ----
		//
		// Per-design pre-serialized response payloads, counted against the SAME
		// byte budget as the archive/Design entries (see LruEntry::serializedBytes).
		// Population: a background pre-serialization is scheduled when a design
		// load completes (synchronous and LoadDesignAsync/M1.2 background loads
		// alike); the first GetDesign RPC that misses also serializes lazily, so
		// behavior is correct even when the background pass lost a race or never
		// ran. At most one serialization is counted per design per invalidation
		// generation.
		//
		// Invalidation (drops the payload entries; next request re-populates):
		// AddFileArchive (re-upload/POST /filemodels — also drops the cached
		// Design, which was built from the old archive), LRU eviction, Clear().
		// A commit racing any of these is discarded (generation + design-identity
		// + epoch checks), so stale bytes can never be published.

		// REST JSON payload variants exposed to the controllers:
		// Design    -> ProductModel::Design::to_json()   (/designs/<name>)
		// FileModel -> FileArchive::to_json()            (/filemodels/<name>)
		// Note: the cached Design JSON is the un-clipped payload (the shared
		// cached Design must keep its file model for the gRPC GetDesign path);
		// the controller wiring decides how/whether to serve it for the default
		// (file-model-clipped) route.
		enum class JsonPayloadKind
		{
			Design,
			FileModel
		};

		// Cached serialized ProductModel::Design bytes (the gRPC GetDesign
		// response payload). True and filled when warm.
		bool TryGetDesignBytes(const std::string& designName, std::string& outBytes) const;

		// Cached REST JSON payload for the design. True and filled when warm.
		bool TryGetJsonPayload(const std::string& designName, JsonPayloadKind kind, std::string& outJson) const;

		// True when a full response payload set is cached for the design.
		bool HasCachedResponse(const std::string& designName) const;

		// Synchronous lazy population: builds and commits the response payloads
		// for the currently cached design (no-op if the design is not cached or
		// was already serialized for the current generation). Used by the gRPC
		// GetDesign fast path's cold branch and by tests.
		void SerializeResponsePayloads(const std::string& designName);

		// Times a response payload set was built and committed (one per design
		// per generation — warm GetDesign hits add nothing). Test seam for the
		// "exactly one to_protobuf call" acceptance criterion.
		std::uint64_t designSerializationCount() const;

		// Total bytes currently charged to serialized payloads (already included
		// in the LRU byte budget). Drops to 0 on eviction/invalidation/Clear().
		std::uint64_t cachedResponseBytes() const;

	private:
		std::string m_directory;

		FileModel::Design::FileArchive::StringMap m_fileArchivesByName;
		ProductModel::Design::StringMap m_designsByName;

		// Protects m_fileArchivesByName, m_designsByName, and m_directory
		// Use shared_lock for reads, unique_lock for writes
		mutable std::shared_mutex m_cacheMutex;

		// ---- single-flight load bookkeeping ----
		//
		// Lock order (never acquired in reverse; allowed nestings are
		// m_lruMutex -> m_loadStateMutex inside eviction and
		// m_lruMutex -> ResponseStore::m_mutex inside payload commit):
		//   m_cacheMutex               -> (nothing)
		//   m_lruMutex                 -> m_loadStateMutex, ResponseStore::m_mutex
		//   ResponseStore::m_mutex     -> (nothing)
		//   m_loadStateMutex           -> (nothing)
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

		// Names with a background load kicked but not yet finished (queued on the
		// semaphore or parsing). Closing the between-kick-and-Loading window
		// synchronously in LoadDesignAsync makes a second kick for the same name
		// deterministically return false instead of racing the background
		// thread's Loading announcement. Guarded by m_loadStateMutex.
		std::unordered_set<std::string> m_pendingAsyncLoads;

		// ---- background-load bookkeeping ----
		//
		// Counting semaphore for background parses (mutex + cv; C++17 target).
		// m_outstandingBackgroundLoads counts spawned-but-unfinished background
		// threads (queued or parsing); ~DesignCache drains it so detached loads
		// never outlive the cache members they touch. Guarded by m_backgroundMutex.
		mutable std::mutex m_backgroundMutex;
		std::condition_variable m_backgroundSlotCv;
		std::condition_variable m_backgroundDoneCv;
		std::size_t m_maxBackgroundLoads = DEFAULT_MAX_BACKGROUND_LOADS;
		std::size_t m_activeBackgroundLoads = 0;        // threads currently holding a parse slot
		std::size_t m_outstandingBackgroundLoads = 0;   // spawned but unfinished threads

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
			// Bytes charged for the design's cached serialized response payloads
			// (included in estimatedBytes' contribution to m_cachedBytes). Kept
			// separately so invalidation can uncharge without dropping the
			// archive/Design cache entry.
			std::uint64_t serializedBytes = 0;
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

		// ---- serialized-response cache (M1.4) ----
		//
		// The pre-serialized response payloads for one design.
		struct ResponsePayload
		{
			std::string designPbBytes;   // serialized ProductModel::Design (gRPC GetDesign response bytes)
			std::string designJson;      // ProductModel::Design::to_json()  (/designs/<name>)
			std::string fileModelJson;   // FileArchive::to_json()           (/filemodels/<name>)

			std::uint64_t totalBytes() const
			{
				return static_cast<std::uint64_t>(designPbBytes.size()) +
					static_cast<std::uint64_t>(designJson.size()) +
					static_cast<std::uint64_t>(fileModelJson.size());
			}
		};

		// Payload store with per-design invalidation generations. A committer
		// captures CurrentGeneration before serializing; TryStore succeeds only
		// while the generation is unchanged, and stores at most once per
		// (design, generation): a duplicate commit for an already-stored
		// generation is a no-op success (this is what makes the design-
		// serialization counter exact under racing background/lazy committers).
		// Self-contained behind m_mutex; never calls back into DesignCache.
		class ResponseStore
		{
		public:
			struct StoreResult
			{
				bool stored = false;            // payload is current for (name, generation)
				std::uint64_t storedBytes = 0;  // bytes now held for the name
			};

			bool TryGetDesignBytes(const std::string& designName, std::string& outBytes) const;
			bool TryGetJsonPayload(const std::string& designName, JsonPayloadKind kind, std::string& outJson) const;
			bool HasEntry(const std::string& designName) const;
			std::uint64_t StoredBytes(const std::string& designName) const;
			std::uint64_t TotalStoredBytes() const;
			std::uint64_t CurrentGeneration(const std::string& designName) const;
			StoreResult TryStore(const std::string& designName, std::uint64_t generation, ResponsePayload&& payload);
			// Drops the entry and bumps the generation; returns the dropped bytes.
			std::uint64_t Invalidate(const std::string& designName);
			void Clear();
			std::uint64_t designSerializations() const;

		private:
			struct Entry
			{
				ResponsePayload payload;
			};

			mutable std::mutex m_mutex;
			std::unordered_map<std::string, Entry> m_entriesByName;
			// Per-design invalidation generation (monotonically increasing
			// tombstones; absent = 0). Single source of truth for TryStore
			// validation — survives entry erasure so a commit that captured a
			// pre-invalidation generation always loses.
			std::unordered_map<std::string, std::uint64_t> m_generationsByName;
			std::uint64_t m_designSerializations = 0;
		};

		ResponseStore m_responseStore;

		// Builds the payload set from a loaded Design. The protobuf tree is
		// built once: the serialized bytes and the Design JSON both come from
		// the same message (no second ProductModel traversal).
		static ResponsePayload BuildResponsePayload(const ProductModel::Design& design);

		// Validates (epoch, LRU presence, Design identity, generation) and
		// commits a payload set, charging its bytes to the design's LRU entry
		// and evicting over budget (never this design). Abort silently leaves
		// the cache unchanged.
		void CommitResponsePayloads(const std::string& designName,
			std::uint64_t epochAtStart,
			const std::shared_ptr<ProductModel::Design>& pDesign,
			ResponsePayload&& payload,
			std::uint64_t generation);

		// Schedules a detached background thread that builds + commits the
		// payloads for a design that just reached Loaded. Counted/drained via
		// m_outstandingBackgroundLoads exactly like background loads.
		void ScheduleResponsePreSerialization(const std::string& designName,
			std::shared_ptr<ProductModel::Design> pDesign,
			std::uint64_t epochAtStart);

		// Body of the background pre-serialization thread. Never lets an
		// exception escape.
		void PreSerializeWorker(std::string designName,
			std::shared_ptr<ProductModel::Design> pDesign,
			std::uint64_t epochAtStart,
			std::uint64_t generation);

		// Drops the payload entry (generation bump) and uncharges its bytes
		// from the design's LRU entry, when one exists.
		void InvalidateResponsePayloads(const std::string& designName);

		// Resyncs one LRU entry's serializedBytes with the store (charge delta).
		// Requires m_lruMutex NOT held.
		void AdjustLruSerializedBytes(const std::string& designName);

		// Current cache generation snapshot (for lazy serialization commits).
		std::uint64_t CurrentEpoch() const;

		std::shared_ptr<ProductModel::Design> LoadDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> LoadFileArchive(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchiveInternal(const std::string& designName, bool updateState);

		// Shared archive-file-by-stem scan: path of the first regular file in the
		// designs directory whose stem matches designName, or an empty path.
		std::filesystem::path FindArchivePath(const std::string& designName) const;

		// Body of a LoadDesignAsync background thread: queues on the semaphore,
		// runs the single-flight load, logs the outcome, then releases its
		// bookkeeping. Never lets an exception escape (thread functions must not).
		void BackgroundLoad(const std::string& designName);
		void AcquireBackgroundSlot();
		void ReleaseBackgroundSlot();

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

			// M1.4: a design just became serveable — schedule background
			// pre-serialization of its response payloads (gRPC GetDesign bytes
			// + REST JSON variants). Only the design-level load hooks this (the
			// archive sub-load runs with updateState=false). The commit is
			// validated against epoch/eviction/invalidation, so races with
			// Clear()/eviction/AddFileArchive discard the payloads.
			if constexpr (std::is_same_v<T, ProductModel::Design>)
			{
				if (updateState)
				{
					ScheduleResponsePreSerialization(designName, pValue, epochAtStart);
				}
			}

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
		constexpr inline static std::size_t DEFAULT_MAX_BACKGROUND_LOADS = 2;
		// Rough per-design overhead added to the archive file size; serialized response
		// sizes are added to this estimate in M1.4 (response cache).
		constexpr inline static std::uint64_t DESIGN_BYTES_OVERHEAD = 4096;

		constexpr inline static const char* DESIGN_EXTENSIONS[] = { "zip", "tgz", "tar.gz", "tar", "gzip" , "gz" };

	};
}
