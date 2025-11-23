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
#include <App/DesignCache.h>
#include <design.pb.h>

#include <grpcpp/server_context.h>

namespace OdbDesignServer
{
    namespace Services
    {

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
