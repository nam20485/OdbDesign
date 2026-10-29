#pragma once

// DesignStreamWalker — emits the ordered DesignChunk sequence of a design
// (M6.2, getdesign-streaming-oom-plan.md) WITHOUT ever materializing the
// monolithic protobuf Design tree.
//
// The emission order follows the settled M6.1 chunk design:
//   1. DesignHeader (Design scalars + bounded FileArchive fields, no steps)
//   2. one StepSkeleton per step (full StepDirectory minus layer features)
//   3. FeatureBatch chunks per (step, layer) in FeaturesFile order
//   4. NetBatch / PackageBatch / ComponentBatch / PartBatch (only when
//      include_normalized_lists — mirroring the monolithic flavour)
//   5. ConnectivityBatch chunks (nets, packages, netMembers, components — the
//      same sections, entries, and order as Design::FillConnectivityMessage)
//   6. NormalizedMapBatch chunks (the four ByName maps; normalized flavour)
//
// Each chunk's submessage is constructed fresh, handed to the sink, and
// released; peak walker memory is one batch. Byte-identity with the
// monolithic Design::to_protobuf output is by construction: every section is
// produced by the same lib-side serialization code (FileArchive/
// StepDirectory skeletons, per-element to_protobuf, Design::AddConnectivity*
// single-entry emitters), and map entries are emitted in the same sorted
// std::map iteration order the monolithic build inserts them in.
//
// DesignStreamAssembler re-applies the chunks to a Design message (the wire-
// equivalence gate and the reference client reassembly recipe).

#include <cstdint>
#include <functional>
#include <set>
#include <string>

#include <design.pb.h>
#include <service.pb.h>

#include "ProductModel/Design.h"

namespace OdbDesignServer
{
    namespace Services
    {
        // Batch sizes bound peak per-chunk memory; they do NOT affect the
        // reassembled bytes (only chunk granularity).
        struct DesignStreamOptions
        {
            std::size_t featureBatchSize = 1000;  // FeatureRecords per FeatureBatch
            std::size_t entryBatchSize = 1000;    // nets/packages/components/parts + connectivity entries per batch
            std::size_t mapBatchSize = 500;       // ByName map entries per NormalizedMapBatch
        };

        class DesignStreamWalker
        {
        public:
            // Receives each emitted chunk; returns false to abort the walk
            // (e.g. gRPC client disconnected).
            using ChunkSink = std::function<bool(const Odb::Grpc::DesignChunk&)>;

            struct Stats
            {
                std::uint64_t chunks = 0;
                std::uint64_t wireBytes = 0;   // sum of serialized chunk sizes
            };

            // Walks the product model and emits the ordered chunk sequence.
            // Returns true when the walk completed; false when the sink
            // aborted it partway. `stats` (optional) receives counters.
            static bool Walk(const Odb::Lib::ProductModel::Design& design,
                bool includeNormalizedLists,
                const DesignStreamOptions& options,
                const ChunkSink& sink,
                Stats* stats = nullptr);
        };

        // Reference reassembly: applies one chunk to the Design message being
        // rebuilt. Chunks must arrive in walk order. Returns false on a
        // malformed sequence (e.g. a FeatureBatch addressing a step/layer no
        // StepSkeleton delivered, or a chunk arriving before the header).
        bool ApplyDesignChunk(Odb::Lib::Protobuf::ProductModel::Design& design,
            const Odb::Grpc::DesignChunk& chunk);
    }
}
