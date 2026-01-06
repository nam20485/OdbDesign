#include "OdbDesignServiceImpl.h"
#include "FileModel/Design/FeaturesFile.h"
#include "FileModel/Design/FileArchive.h"
#include <utility>
#include <odbdesign_export.h>
#include <featuresfile.pb.h>
#include <grpcpp/impl/status.h>
#include <string>
#include <exception>
#include <grpcpp/support/sync_stream.h>
#include <grpcpp/support/status_code_enum.h>
#include <memory>
#include <algorithm>
#include <cctype>
#include <App/DesignCache.h>
#include <design.pb.h>
#include <vector>

#include <grpcpp/server_context.h>

namespace OdbDesignServer
{
    namespace Services
    {

        namespace
        {
            struct UnitsInfo
            {
                std::string units;
                double unitsToMm{};
                bool ok{ false };
                std::string error;
            };

            std::string to_lower_copy(const std::string& s)
            {
                std::string out = s;
                std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return out;
            }

            UnitsInfo normalize_units(std::string rawUnits)
            {
                UnitsInfo info{};
                if (rawUnits.empty())
                {
                    info.error = "layer units not set";
                    return info;
                }

                auto u = to_lower_copy(rawUnits);
                if (u == "inch" || u == "inches" || u == "in")
                {
                    info.units = "inch";
                    info.unitsToMm = 25.4;
                    info.ok = true;
                }
                else if (u == "mm" || u == "millimeter" || u == "millimeters")
                {
                    info.units = "mm";
                    info.unitsToMm = 1.0;
                    info.ok = true;
                }
                else if (u == "mil" || u == "mils")
                {
                    info.units = "mil";
                    info.unitsToMm = 0.0254;
                    info.ok = true;
                }
                else if (u == "micron" || u == "microns" || u == "um" || u == "µm")
                {
                    info.units = "micron";
                    info.unitsToMm = 0.001;
                    info.ok = true;
                }
                else
                {
                    info.error = "unsupported units: " + rawUnits;
                }

                return info;
            }

            Odb::Lib::UnitType expected_symbol_unit_type_from_normalized_units(const std::string& normalizedUnits)
            {
                // Contract requires symbol numeric parameters to be in same unit system as feature coordinates.
                auto u = to_lower_copy(normalizedUnits);
                if (u == "mm" || u == "micron")
                {
                    return Odb::Lib::UnitType::Metric;
                }
                if (u == "inch" || u == "mil")
                {
                    return Odb::Lib::UnitType::Imperial;
                }
                return Odb::Lib::UnitType::None;
            }

        }


        OdbDesignServiceImpl::OdbDesignServiceImpl(std::shared_ptr<Odb::Lib::App::DesignCache> cache,
            std::shared_ptr<Config::GrpcServiceConfig> config)
            : m_designCache(std::move(cache)),
              m_config(config ? config : Config::GrpcServiceConfig::CreateDefault()) {}

        grpc::Status OdbDesignServiceImpl::GetDesign(
            grpc::ServerContext *context,
            const Odb::Grpc::GetDesignRequest *request,
            Odb::Lib::Protobuf::ProductModel::Design *response)
        {
            try
            {
                const auto design = m_designCache->GetDesign(request->design_name());
                if (design == nullptr)
                {
                    return {grpc::StatusCode::NOT_FOUND, "Design not found: " + request->design_name()};
                }

                // Convert the design to protobuf message and populate the response
                *response = *(design->to_protobuf());
                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return {grpc::StatusCode::INTERNAL, error};
            }
        }

        grpc::Status OdbDesignServiceImpl::GetLayerFeaturesStream(
            grpc::ServerContext *context,
            const Odb::Grpc::GetLayerFeaturesRequest *request,
            grpc::ServerWriter<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> *writer)
        {
            try
            {
                const auto fileArchive = m_designCache->GetFileArchive(request->design_name());
                if (fileArchive == nullptr)
                {
                    return {grpc::StatusCode::NOT_FOUND, "Design not found"};
                }

                const auto pStep = fileArchive->GetStepDirectory(request->step_name());
                if (pStep == nullptr)
                {
                    return {grpc::StatusCode::NOT_FOUND, "Step not found"};
                }

                const auto pLayersByName = pStep->GetLayersByName();
                const auto layerIt = pLayersByName.find(request->layer_name());
                if (layerIt == pLayersByName.end())
                {
                    return {grpc::StatusCode::NOT_FOUND, "Layer not found"};
                }

                const auto &featuresFile = layerIt->second->GetFeaturesFile();
                const auto &featureRecords = featuresFile.GetFeatureRecords();

                // Reuse a single protobuf message instance to avoid per-iteration heap allocations.
                Odb::Lib::Protobuf::FeaturesFile::FeatureRecord featureRecordMsg;

                for (const auto &featureRecord : featureRecords)
                {
                    if (!featureRecord)
                        continue;

                    // Populate reused message: obtain per-feature proto and swap into reusable instance.
                    auto pFeatureRecordMsg = featureRecord->to_protobuf();
                    if (pFeatureRecordMsg != nullptr)
                    {
                        featureRecordMsg.Swap(pFeatureRecordMsg.get()); // Efficient move of fields.
                    }
                    else
                    {
                        continue; // Skip if conversion failed or returned nullptr.
                    }

                    if (!writer->Write(featureRecordMsg))
                    {
                        // Client disconnected
                        break;
                    }

                    // Clear for reuse while retaining allocated capacity.
                    featureRecordMsg.Clear();
                }

                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return {grpc::StatusCode::INTERNAL, error};
            }
        }

        grpc::Status OdbDesignServiceImpl::GetLayerFeaturesBatchStream(
            grpc::ServerContext *context,
            const Odb::Grpc::GetLayerFeaturesRequest *request,
            grpc::ServerWriter<Odb::Grpc::FeatureRecordBatch> *writer)
        {
            try
            {
                // Feature flag check: return UNIMPLEMENTED if batch streaming is disabled
                if (!m_config->enable_batch_streaming)
                {
                    return {grpc::StatusCode::UNIMPLEMENTED, "Batch streaming is not enabled"};
                }

                // Reuse validation logic from GetLayerFeaturesStream
                const auto fileArchive = m_designCache->GetFileArchive(request->design_name());
                if (fileArchive == nullptr)
                {
                    return {grpc::StatusCode::NOT_FOUND, "Design not found"};
                }

                const auto pStep = fileArchive->GetStepDirectory(request->step_name());
                if (pStep == nullptr)
                {
                    return {grpc::StatusCode::NOT_FOUND, "Step not found"};
                }

                const auto pLayersByName = pStep->GetLayersByName();
                const auto layerIt = pLayersByName.find(request->layer_name());
                if (layerIt == pLayersByName.end())
                {
                    return {grpc::StatusCode::NOT_FOUND, "Layer not found"};
                }

                const auto &featuresFile = layerIt->second->GetFeaturesFile();
                const auto &featureRecords = featuresFile.GetFeatureRecords();

                // Reuse a single protobuf message instance to avoid per-iteration heap allocations.
                Odb::Lib::Protobuf::FeaturesFile::FeatureRecord featureRecordMsg;
                Odb::Grpc::FeatureRecordBatch batch;

                const int batchSize = m_config->batch_size;
                int currentBatchCount = 0;

                for (const auto &featureRecord : featureRecords)
                {
                    // Check for cancellation
                    if (context->IsCancelled())
                    {
                        return {grpc::StatusCode::CANCELLED, "Request cancelled"};
                    }

                    if (!featureRecord)
                        continue;

                    // Populate reused message: obtain per-feature proto and swap into reusable instance.
                    auto pFeatureRecordMsg = featureRecord->to_protobuf();
                    if (pFeatureRecordMsg != nullptr)
                    {
                        featureRecordMsg.Swap(pFeatureRecordMsg.get()); // Efficient move of fields.
                    }
                    else
                    {
                        continue; // Skip if conversion failed or returned nullptr.
                    }

                    // Add feature to current batch
                    batch.add_features()->CopyFrom(featureRecordMsg);
                    currentBatchCount++;

                    // Clear for reuse while retaining allocated capacity.
                    featureRecordMsg.Clear();

                    // Send batch when it reaches the configured size
                    if (currentBatchCount >= batchSize)
                    {
                        if (!writer->Write(batch))
                        {
                            // Client disconnected during batch write
                            std::cerr << "DEBUG: Client disconnected while sending batch" << std::endl;
                            return grpc::Status::OK;
                        }
                        batch.Clear();
                        currentBatchCount = 0;
                    }
                }

                // Send final batch if there are remaining features
                if (currentBatchCount > 0)
                {
                    if (!writer->Write(batch))
                    {
                        // Client disconnected during final batch write
                        std::cerr << "DEBUG: Client disconnected while sending final batch" << std::endl;
                        return grpc::Status::OK;
                    }
                }

                // Stream completion is indicated by normal gRPC end-of-stream semantics
                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return {grpc::StatusCode::INTERNAL, error};
            }
        }

        grpc::Status OdbDesignServiceImpl::GetLayerSymbols(
            grpc::ServerContext* context,
            const Odb::Grpc::GetLayerSymbolsRequest* request,
            Odb::Grpc::GetLayerSymbolsResponse* response)
        {
            try
            {
                const auto fileArchive = m_designCache->GetFileArchive(request->design_name());
                if (fileArchive == nullptr)
                {
                    return { grpc::StatusCode::NOT_FOUND, "Design not found" };
                }

                const auto pStep = fileArchive->GetStepDirectory(request->step_name());
                if (pStep == nullptr)
                {
                    return { grpc::StatusCode::NOT_FOUND, "Step not found" };
                }

                const auto& layersByName = pStep->GetLayersByName();
                const auto layerIt = layersByName.find(request->layer_name());
                if (layerIt == layersByName.end())
                {
                    return { grpc::StatusCode::NOT_FOUND, "Layer not found" };
                }

                const auto& featuresFile = layerIt->second->GetFeaturesFile();
                auto unitsInfo = normalize_units(featuresFile.GetUnits());
                if (!unitsInfo.ok)
                {
                    return { grpc::StatusCode::FAILED_PRECONDITION, unitsInfo.error };
                }

                response->set_sym_num_base(0);
                response->set_units(unitsInfo.units);
                response->set_units_to_mm(unitsInfo.unitsToMm);

                const auto symbols = Odb::Lib::FileModel::Design::collect_symbols(featuresFile);
                const auto& featureRecords = featuresFile.GetFeatureRecords();

                // Strict contract enforcement (S2): symbol unit types must match the layer units.
                const auto expectedSymbolUnitType = expected_symbol_unit_type_from_normalized_units(unitsInfo.units);
                int metricCount = 0;
                int imperialCount = 0;
                int noneCount = 0;
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    switch (sym->GetUnitType())
                    {
                    case Odb::Lib::UnitType::Metric: metricCount++; break;
                    case Odb::Lib::UnitType::Imperial: imperialCount++; break;
                    case Odb::Lib::UnitType::None: noneCount++; break;
                    }
                }

                const bool hasMixedSymbolUnitTypes = metricCount > 0 && imperialCount > 0;
                // Spec: if the symbol record omits its unit, it inherits the enclosing features file unit system.
                // So UnitType::None is treated as compatible with the layer units.
                const bool hasMismatchVsLayerUnits =
                    (expectedSymbolUnitType == Odb::Lib::UnitType::Metric && imperialCount > 0) ||
                    (expectedSymbolUnitType == Odb::Lib::UnitType::Imperial && metricCount > 0);

                if (hasMixedSymbolUnitTypes || hasMismatchVsLayerUnits)
                {
                    std::string msg =
                        "symbol UnitType mismatch vs layer units (strict): layer_units=" + unitsInfo.units +
                        ", symbol_counts(metric=" + std::to_string(metricCount) +
                        ", imperial=" + std::to_string(imperialCount) +
                        ", none=" + std::to_string(noneCount) + ")";
                    return { grpc::StatusCode::FAILED_PRECONDITION, msg };
                }

                // Sanity check: limit symbol indices to avoid excessive memory allocation or logic errors due to invalid/corrupted data.
                // 10,000,000 is chosen as a very high upper bound, far exceeding any realistic symbol count in production designs.
                // If a valid symbol index exceeds this limit, it will be ignored, which may cause rendering or data issues.
                // Adjust this value with caution and ensure all code using symbol indices is reviewed if changed.
                constexpr int kMaxReasonableIndex = 10'000'000;
                int maxSymRef = -1;
                // Single pass to validate pad contracts and capture the max referenced symbol id without extra traversals.
                for (const auto& fr : featureRecords)
                {
                    if (!fr) continue;

                    // Strict contract enforcement (S1): Pads must reference symbols via FeatureRecord.sym_num.
                    if (fr->type == Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord::Type::Pad &&
                        fr->apt_def_symbol_num >= 0 && fr->sym_num != fr->apt_def_symbol_num)
                    {
                        return { grpc::StatusCode::FAILED_PRECONDITION, "pad sym_num does not match apt_def_symbol_num (contract violation)" };
                    }

                    if (fr->sym_num >= 0 && fr->sym_num < kMaxReasonableIndex)
                    {
                        maxSymRef = std::max(maxSymRef, fr->sym_num);
                    }
                    if (fr->apt_def_symbol_num >= 0 && fr->apt_def_symbol_num < kMaxReasonableIndex)
                    {
                        maxSymRef = std::max(maxSymRef, fr->apt_def_symbol_num);
                    }
                }

                int resultSize = maxSymRef + 1;
                if (resultSize < 0) resultSize = 0;

                std::vector<std::shared_ptr<Odb::Lib::FileModel::Design::SymbolName>> ordered(static_cast<size_t>(resultSize), nullptr);

                // Ordering strategy:
                // 1) Place symbols that declare explicit indices; keep the first occurrence on duplicates.
                // 2) Fill remaining gaps with symbols lacking indices in deterministic input order.
                // This preserves stable ordering for callers that rely on sym_num alignment.
                int maxIndex = -1;
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    const auto idx = sym->GetIndex();
                    maxIndex = std::max(maxIndex, idx);
                    if (idx < 0) continue;
                    const auto idxSize = static_cast<size_t>(idx);
                    if (idx >= static_cast<int>(ordered.size()))
                    {
                        ordered.resize(idxSize + 1, nullptr);
                    }
                    if (!ordered[idxSize])
                    {
                        ordered[idxSize] = sym;
                    }
                }

                // Then, fill remaining slots with symbols lacking indices in deterministic order.
                // Precompute available slots for O(n) filling instead of O(n²) linear search.
                std::vector<size_t> availableSlots;
                for (size_t i = 0; i < ordered.size(); ++i)
                {
                    if (!ordered[i])
                    {
                        availableSlots.push_back(i);
                    }
                }
                size_t slotIdx = 0;
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    if (sym->GetIndex() >= 0) continue;

                    if (slotIdx < availableSlots.size())
                    {
                        ordered[availableSlots[slotIdx]] = sym;
                        ++slotIdx;
                    }
                    else
                    {
                        ordered.push_back(sym);
                    }
                }

                // Pad to cover all referenced sym_num values.
                const int requiredSize = std::max(maxSymRef, maxIndex) + 1;
                if (requiredSize > static_cast<int>(ordered.size()))
                {
                    ordered.resize(static_cast<size_t>(requiredSize), nullptr);
                }

                Odb::Lib::Protobuf::SymbolName placeholder;
                placeholder.set_name("");
                placeholder.set_unittype(Odb::Lib::Protobuf::UnitType::None);

                for (const auto& sym : ordered)
                {
                    if (sym)
                    {
                        auto pb = sym->to_protobuf();
                        if (pb && pb->unittype() == Odb::Lib::Protobuf::UnitType::None)
                        {
                            pb->set_unittype(static_cast<Odb::Lib::Protobuf::UnitType>(expectedSymbolUnitType));
                        }
                        response->add_symbol_names()->CopyFrom(*pb);
                    }
                    else
                    {
                        response->add_symbol_names()->CopyFrom(placeholder);
                    }
                }

                return grpc::Status::OK;
            }
            catch (const std::exception& e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return { grpc::StatusCode::INTERNAL, error };
            }
        }

        grpc::Status OdbDesignServiceImpl::HealthCheck(
            grpc::ServerContext *context,
            const Odb::Grpc::HealthCheckRequest *request,
            Odb::Grpc::HealthCheckResponse *response)
        {
            try
            {
                // Optionally, check the service name from the request
                if (!request->service().empty() && request->service() != "OdbDesignService")
                {
                    response->set_status(Odb::Grpc::HealthCheckResponse::NOT_SERVING);
                    return grpc::Status::OK;
                }

                response->set_status(Odb::Grpc::HealthCheckResponse::SERVING);
                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return {grpc::StatusCode::INTERNAL, error};
            }
        }

    } // namespace Services
} // namespace OdbDesignServer
