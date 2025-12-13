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

            // Extract symbols as a vector irrespective of whether the source is a vector or map.
            Odb::Lib::FileModel::Design::SymbolName::Vector collect_symbols(const Odb::Lib::FileModel::Design::FeaturesFile& featuresFile)
            {
                const auto& vec = featuresFile.GetSymbolNames();
                if (!vec.empty()) return vec;

                Odb::Lib::FileModel::Design::SymbolName::Vector collected;
                for (const auto& kv : featuresFile.GetSymbolNamesByName())
                {
                    collected.push_back(kv.second);
                }
                return collected;
            }
        }


        OdbDesignServiceImpl::OdbDesignServiceImpl(std::shared_ptr<Odb::Lib::App::DesignCache> cache)
            : m_designCache(std::move(cache)) {}

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

                const auto symbols = collect_symbols(featuresFile);
                const auto& featureRecords = featuresFile.GetFeatureRecords();

                constexpr int kMaxReasonableIndex = 10'000'000;
                int maxSymRef = -1;
                for (const auto& fr : featureRecords)
                {
                    if (!fr) continue;
                    if (fr->sym_num >= 0 && fr->sym_num < kMaxReasonableIndex)
                    {
                        maxSymRef = std::max(maxSymRef, fr->sym_num);
                    }
                    if (fr->apt_def_symbol_num >= 0 && fr->apt_def_symbol_num < kMaxReasonableIndex)
                    {
                        maxSymRef = std::max(maxSymRef, fr->apt_def_symbol_num);
                    }
                }

                int maxIndex = -1;
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    maxIndex = std::max(maxIndex, sym->GetIndex());
                }

                int resultSize = std::max(maxSymRef, maxIndex) + 1;
                if (resultSize < 0) resultSize = 0;

                using SymbolPtr = std::shared_ptr<Odb::Lib::FileModel::Design::SymbolName>;
                std::vector<SymbolPtr> ordered(static_cast<size_t>(resultSize), nullptr);

                // First, place all symbols with explicit indices.
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    auto idx = sym->GetIndex();
                    if (idx < 0) continue;
                    if (idx >= static_cast<int>(ordered.size()))
                    {
                        ordered.resize(static_cast<size_t>(idx + 1), nullptr);
                    }
                    if (!ordered[static_cast<size_t>(idx)])
                    {
                        ordered[static_cast<size_t>(idx)] = sym;
                    }
                }

                // Then, fill remaining slots with symbols lacking indices in deterministic order.
                int nextSlot = 0;
                for (const auto& sym : symbols)
                {
                    if (!sym) continue;
                    if (sym->GetIndex() >= 0) continue;

                    while (nextSlot < static_cast<int>(ordered.size()) && ordered[static_cast<size_t>(nextSlot)])
                    {
                        nextSlot++;
                    }

                    if (nextSlot >= static_cast<int>(ordered.size()))
                    {
                        ordered.push_back(sym);
                        nextSlot = static_cast<int>(ordered.size());
                    }
                    else
                    {
                        ordered[static_cast<size_t>(nextSlot)] = sym;
                        nextSlot++;
                    }
                }

                // Pad to cover all referenced sym_num values.
                if (resultSize > static_cast<int>(ordered.size()))
                {
                    ordered.resize(static_cast<size_t>(resultSize), nullptr);
                }

                Odb::Lib::Protobuf::SymbolName placeholder;
                placeholder.set_name("");
                placeholder.set_unittype(Odb::Lib::Protobuf::UnitType::None);

                for (const auto& sym : ordered)
                {
                    if (sym)
                    {
                        auto pb = sym->to_protobuf();
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
