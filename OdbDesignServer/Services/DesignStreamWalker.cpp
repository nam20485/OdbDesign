#include "DesignStreamWalker.h"
#include <Logger.h>
#include <Text/Utf8Sanitizer.h>

namespace OdbDesignServer
{
    namespace Services
    {
        using Odb::Lib::ProductModel::Design;

        // Streams one Design ByName map as NormalizedMapBatch chunks with the
        // same first-wins sanitized-key semantics as
        // FillProtobufMapWithSanitizedKeys. The collision set is local to the
        // map (refDes and net names legitimately overlap across the four
        // collections; the monolithic build dedupes within each map field
        // independently).
        template <typename TEntries, typename TMapGetter>
        bool StreamNormalizedMap(const TEntries& entries,
            std::size_t mapBatchSize,
            TMapGetter getMapField,
            const char* mapName,
            const DesignStreamWalker::ChunkSink& emit)
        {
            Odb::Grpc::DesignChunk chunk;
            auto* batch = chunk.mutable_normalized_maps();
            std::set<std::string> emittedKeys;
            std::size_t count = 0;
            for (const auto& kv : entries)
            {
                const auto key = Odb::Lib::Text::ToUtf8(kv.first);
                if (emittedKeys.count(key) != 0)
                {
                    logwarn(std::string("UTF-8 sanitization collision in streamed protobuf map (") + mapName +
                        "): key \"" + kv.first + "\" sanitizes to a key that is already present; keeping the first entry");
                    continue;
                }
                emittedKeys.insert(key);
                auto* mapField = ((*batch).*getMapField)();
                (*mapField)[key] = *kv.second->to_protobuf();
                ++count;
                if (count >= mapBatchSize)
                {
                    if (!emit(chunk))
                    {
                        return false;
                    }
                    chunk.Clear();
                    batch = chunk.mutable_normalized_maps();
                    count = 0;
                }
            }
            if (count > 0)
            {
                return emit(chunk);
            }
            return true;
        }

        bool DesignStreamWalker::Walk(const Design& design,
            bool includeNormalizedLists,
            const DesignStreamOptions& options,
            const ChunkSink& sink,
            Stats* stats)
        {
            auto emit = [&sink, stats](const Odb::Grpc::DesignChunk& chunk)
            {
                if (stats != nullptr)
                {
                    ++stats->chunks;
                    stats->wireBytes += chunk.ByteSizeLong();
                }
                return sink(chunk);
            };

            // ---- 1. DesignHeader ----
            {
                Odb::Grpc::DesignChunk chunk;
                auto* header = chunk.mutable_header();
                header->set_product_model(Odb::Lib::Text::ToUtf8(design.GetProductModel()));
                header->set_name(Odb::Lib::Text::ToUtf8(design.GetName()));
                const auto fileModel = design.GetFileModel();
                if (fileModel != nullptr)
                {
                    auto fileArchiveMsg = fileModel->to_protobuf_skeleton();
                    header->mutable_file_archive()->Swap(fileArchiveMsg.get());
                }
                if (!emit(chunk))
                {
                    return false;
                }
            }

            const auto fileModel = design.GetFileModel();
            if (fileModel != nullptr)
            {
                // ---- 2. StepSkeletons (stepsByName key order — the same
                // order FileArchive::to_protobuf inserts the map entries in) ----
                for (const auto& kvStep : fileModel->GetStepsByName())
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* stepChunk = chunk.mutable_step();
                    stepChunk->set_step_name(Odb::Lib::Text::ToUtf8(kvStep.first));
                    auto stepMsg = kvStep.second->to_protobuf_skeleton();
                    stepChunk->mutable_step()->Swap(stepMsg.get());
                    if (!emit(chunk))
                    {
                        return false;
                    }
                }

                // ---- 3. FeatureBatches (step order, layer order, record
                // order — FeaturesFile::to_protobuf's exact traversal) ----
                for (const auto& kvStep : fileModel->GetStepsByName())
                {
                    for (const auto& kvLayer : kvStep.second->GetLayersByName())
                    {
                        const auto& records = kvLayer.second->GetFeaturesFile().GetFeatureRecords();
                        if (records.empty())
                        {
                            continue;
                        }

                        Odb::Grpc::DesignChunk chunk;
                        auto* batch = chunk.mutable_features();
                        batch->set_step_name(Odb::Lib::Text::ToUtf8(kvStep.first));
                        batch->set_layer_name(Odb::Lib::Text::ToUtf8(kvLayer.first));
                        for (const auto& pRecord : records)
                        {
                            // Identical to FeaturesFile::to_protobuf: no null
                            // filtering, same per-record serialization.
                            batch->add_records()->CopyFrom(*pRecord->to_protobuf());
                            if (static_cast<std::size_t>(batch->records_size()) >= options.featureBatchSize)
                            {
                                if (!emit(chunk))
                                {
                                    return false;
                                }
                                chunk.Clear();
                                batch = chunk.mutable_features();
                                batch->set_step_name(Odb::Lib::Text::ToUtf8(kvStep.first));
                                batch->set_layer_name(Odb::Lib::Text::ToUtf8(kvLayer.first));
                            }
                        }
                        if (batch->records_size() > 0)
                        {
                            if (!emit(chunk))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            if (includeNormalizedLists)
            {
                // ---- 4. Design-level repeated batches (same order and
                // per-element serialization as Design::to_protobuf) ----
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* batch = chunk.mutable_nets();
                    for (const auto& pNet : design.GetNets())
                    {
                        batch->add_nets()->CopyFrom(*pNet->to_protobuf());
                        if (static_cast<std::size_t>(batch->nets_size()) >= options.entryBatchSize)
                        {
                            if (!emit(chunk)) return false;
                            chunk.Clear();
                            batch = chunk.mutable_nets();
                        }
                    }
                    if (batch->nets_size() > 0)
                    {
                        if (!emit(chunk)) return false;
                    }
                }
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* batch = chunk.mutable_packages();
                    for (const auto& pPackage : design.GetPackages())
                    {
                        batch->add_packages()->CopyFrom(*pPackage->to_protobuf());
                        if (static_cast<std::size_t>(batch->packages_size()) >= options.entryBatchSize)
                        {
                            if (!emit(chunk)) return false;
                            chunk.Clear();
                            batch = chunk.mutable_packages();
                        }
                    }
                    if (batch->packages_size() > 0)
                    {
                        if (!emit(chunk)) return false;
                    }
                }
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* batch = chunk.mutable_components();
                    for (const auto& pComponent : design.GetComponents())
                    {
                        batch->add_components()->CopyFrom(*pComponent->to_protobuf());
                        if (static_cast<std::size_t>(batch->components_size()) >= options.entryBatchSize)
                        {
                            if (!emit(chunk)) return false;
                            chunk.Clear();
                            batch = chunk.mutable_components();
                        }
                    }
                    if (batch->components_size() > 0)
                    {
                        if (!emit(chunk)) return false;
                    }
                }
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* batch = chunk.mutable_parts();
                    for (const auto& pPart : design.GetParts())
                    {
                        batch->add_parts()->CopyFrom(*pPart->to_protobuf());
                        if (static_cast<std::size_t>(batch->parts_size()) >= options.entryBatchSize)
                        {
                            if (!emit(chunk)) return false;
                            chunk.Clear();
                            batch = chunk.mutable_parts();
                        }
                    }
                    if (batch->parts_size() > 0)
                    {
                        if (!emit(chunk)) return false;
                    }
                }
            }

            // ---- 5. Connectivity batches (sections in FillConnectivityMessage
            // order: nets, packages, netMembers, components; entries emitted by
            // the same Design::AddConnectivity* single-entry emitters) ----
            {
                const auto index = design.BuildConnectivityIndex();

                // FillConnectivityMessage runs unconditionally (D4), so the
                // monolith always serializes field 12 — a design with no
                // nets/packages/components as present-but-empty. Track whether
                // any ConnectivityBatch was emitted so a no-net design still
                // emits one (empty) batch and reassembly creates the
                // submessage; otherwise the reassembled message would lack
                // field 12 entirely and diverge from the monolithic bytes.
                bool connectivityEmitted = false;
                auto emitConnectivity = [&emit, &connectivityEmitted](const Odb::Grpc::DesignChunk& chunk)
                {
                    connectivityEmitted = true;
                    return emit(chunk);
                };

                // nets
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* entries = chunk.mutable_connectivity()->mutable_entries();
                    for (std::size_t i = 0; i < design.GetNets().size(); ++i)
                    {
                        design.AddConnectivityNetEntry(i, *entries);
                        if (static_cast<std::size_t>(entries->nets_size()) >= options.entryBatchSize)
                        {
                            if (!emitConnectivity(chunk)) return false;
                            chunk.Clear();
                            entries = chunk.mutable_connectivity()->mutable_entries();
                        }
                    }
                    if (entries->nets_size() > 0)
                    {
                        if (!emitConnectivity(chunk)) return false;
                    }
                }
                // packages
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* entries = chunk.mutable_connectivity()->mutable_entries();
                    for (std::size_t i = 0; i < design.GetPackages().size(); ++i)
                    {
                        design.AddConnectivityPackagePinEntry(i, *entries);
                        if (static_cast<std::size_t>(entries->packages_size()) >= options.entryBatchSize)
                        {
                            if (!emitConnectivity(chunk)) return false;
                            chunk.Clear();
                            entries = chunk.mutable_connectivity()->mutable_entries();
                        }
                    }
                    if (entries->packages_size() > 0)
                    {
                        if (!emitConnectivity(chunk)) return false;
                    }
                }
                // netMembers
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* entries = chunk.mutable_connectivity()->mutable_entries();
                    for (auto membersIt = index.membersByNet.begin(); membersIt != index.membersByNet.end(); ++membersIt)
                    {
                        design.AddConnectivityNetMemberEntry(index, membersIt, *entries);
                        if (static_cast<std::size_t>(entries->netmembers_size()) >= options.entryBatchSize)
                        {
                            if (!emitConnectivity(chunk)) return false;
                            chunk.Clear();
                            entries = chunk.mutable_connectivity()->mutable_entries();
                        }
                    }
                    if (entries->netmembers_size() > 0)
                    {
                        if (!emitConnectivity(chunk)) return false;
                    }
                }
                // components
                {
                    Odb::Grpc::DesignChunk chunk;
                    auto* entries = chunk.mutable_connectivity()->mutable_entries();
                    for (std::size_t i = 0; i < design.GetComponents().size(); ++i)
                    {
                        design.AddConnectivityPlacedComponentEntry(index, i, *entries);
                        if (static_cast<std::size_t>(entries->components_size()) >= options.entryBatchSize)
                        {
                            if (!emitConnectivity(chunk)) return false;
                            chunk.Clear();
                            entries = chunk.mutable_connectivity()->mutable_entries();
                        }
                    }
                    if (entries->components_size() > 0)
                    {
                        if (!emitConnectivity(chunk)) return false;
                    }
                }

                if (!connectivityEmitted)
                {
                    // Present-but-empty parity: ApplyDesignChunk's
                    // mutable_connectivity() creates the submessage even for an
                    // empty entries payload, matching the monolith's tag+len(0).
                    Odb::Grpc::DesignChunk chunk;
                    chunk.mutable_connectivity();
                    if (!emit(chunk))
                    {
                        return false;
                    }
                }
            }

            if (includeNormalizedLists)
            {
                // ---- 6. NormalizedMapBatch chunks. Same first-wins sanitized
                // key semantics as FillProtobufMapWithSanitizedKeys: entries
                // are emitted in std::map (sorted-key) order — the order the
                // monolithic build inserts them — and a raw key whose sanitized
                // form collides with an already-emitted key is skipped. The
                // collision set is PER MAP (refDes and net names legitimately
                // overlap across the four collections; the monolithic build
                // dedupes within each map field independently). ----
                if (!StreamNormalizedMap(design.GetNetsByName(), options.mapBatchSize,
                        &Odb::Grpc::NormalizedMapBatch::mutable_nets_by_name, "netsByName", emit))
                {
                    return false;
                }
                if (!StreamNormalizedMap(design.GetPackagesByName(), options.mapBatchSize,
                        &Odb::Grpc::NormalizedMapBatch::mutable_packages_by_name, "packagesByName", emit))
                {
                    return false;
                }
                if (!StreamNormalizedMap(design.GetComponentsByName(), options.mapBatchSize,
                        &Odb::Grpc::NormalizedMapBatch::mutable_components_by_name, "componentsByName", emit))
                {
                    return false;
                }
                if (!StreamNormalizedMap(design.GetPartsByName(), options.mapBatchSize,
                        &Odb::Grpc::NormalizedMapBatch::mutable_parts_by_name, "partsByName", emit))
                {
                    return false;
                }
            }

            return true;
        }

        bool ApplyDesignChunk(Odb::Lib::Protobuf::ProductModel::Design& design,
            const Odb::Grpc::DesignChunk& chunk)
        {
            switch (chunk.payload_case())
            {
            case Odb::Grpc::DesignChunk::kHeader:
                design.set_productmodel(chunk.header().product_model());
                design.set_name(chunk.header().name());
                // Presence parity with Design::to_protobuf: the monolith sets
                // filemodel only when the design has a file model, so the
                // header chunk carries it only then (Walk emits none for a
                // null/clipped file model) — never create an empty one.
                if (chunk.header().has_file_archive())
                {
                    design.mutable_filemodel()->CopyFrom(chunk.header().file_archive());
                }
                return true;

            case Odb::Grpc::DesignChunk::kStep:
            {
                if (!design.has_filemodel())
                {
                    return false;
                }
                const auto& step = chunk.step();
                (*design.mutable_filemodel()->mutable_stepsbyname())[step.step_name()].CopyFrom(step.step());
                return true;
            }

            case Odb::Grpc::DesignChunk::kFeatures:
            {
                if (!design.has_filemodel())
                {
                    return false;
                }
                const auto& batch = chunk.features();
                auto* stepsByName = design.mutable_filemodel()->mutable_stepsbyname();
                auto stepIt = stepsByName->find(batch.step_name());
                if (stepIt == stepsByName->end())
                {
                    return false;
                }
                auto* layersByName = stepIt->second.mutable_layersbyname();
                auto layerIt = layersByName->find(batch.layer_name());
                if (layerIt == layersByName->end())
                {
                    return false;
                }
                auto* featureFile = layerIt->second.mutable_featurefile();
                for (const auto& record : batch.records())
                {
                    featureFile->add_featurerecords()->CopyFrom(record);
                }
                return true;
            }

            case Odb::Grpc::DesignChunk::kNets:
                for (const auto& net : chunk.nets().nets())
                {
                    design.add_nets()->CopyFrom(net);
                }
                return true;

            case Odb::Grpc::DesignChunk::kPackages:
                for (const auto& packageEntry : chunk.packages().packages())
                {
                    design.add_packages()->CopyFrom(packageEntry);
                }
                return true;

            case Odb::Grpc::DesignChunk::kComponents:
                for (const auto& component : chunk.components().components())
                {
                    design.add_components()->CopyFrom(component);
                }
                return true;

            case Odb::Grpc::DesignChunk::kParts:
                for (const auto& part : chunk.parts().parts())
                {
                    design.add_parts()->CopyFrom(part);
                }
                return true;

            case Odb::Grpc::DesignChunk::kConnectivity:
                design.mutable_connectivity()->MergeFrom(chunk.connectivity().entries());
                return true;

            case Odb::Grpc::DesignChunk::kNormalizedMaps:
            {
                const auto& maps = chunk.normalized_maps();
                for (const auto& kv : maps.nets_by_name())
                {
                    (*design.mutable_netsbyname())[kv.first] = kv.second;
                }
                for (const auto& kv : maps.packages_by_name())
                {
                    (*design.mutable_packagesbyname())[kv.first] = kv.second;
                }
                for (const auto& kv : maps.components_by_name())
                {
                    (*design.mutable_componentsbyname())[kv.first] = kv.second;
                }
                for (const auto& kv : maps.parts_by_name())
                {
                    (*design.mutable_partsbyname())[kv.first] = kv.second;
                }
                return true;
            }

            case Odb::Grpc::DesignChunk::PAYLOAD_NOT_SET:
            default:
                return false;
            }
        }
    }
}
