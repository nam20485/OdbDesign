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
#include <grpc/service.pb.h>
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
                auto design = m_designCache->GetDesign(request->design_name());
                if (design == nullptr)
                {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Design not found: " + request->design_name());
                }

                // Convert the design to protobuf message
                design->to_protobuf();
                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return grpc::Status(grpc::StatusCode::INTERNAL, error);
            }
        }

        grpc::Status OdbDesignServiceImpl::GetLayerFeaturesStream(
            grpc::ServerContext *context,
            const Odb::Grpc::GetLayerFeaturesRequest *request,
            grpc::ServerWriter<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> *writer)
        {
            try
            {
                auto fileArchive = m_designCache->GetFileArchive(request->design_name());
                if (fileArchive == nullptr)
                {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Design not found");
                }

                auto pStep = fileArchive->GetStepDirectory(request->step_name());
                if (pStep == nullptr)
                {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Step not found");
                }

                auto pLayersByName = pStep->GetLayersByName();
                auto layerIt = pLayersByName.find(request->layer_name());
                if (layerIt == pLayersByName.end())
                {
                    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Layer not found");
                }

                const auto &featuresFile = layerIt->second->GetFeaturesFile();
                const auto &featureRecords = featuresFile.GetFeatureRecords();
                for (const auto &featureRecord : featureRecords)
                {
                    if (featureRecord == nullptr)
                        continue;

                    // Efficiently populate the *reused* message
                    // This avoids heap allocation inside the loop.
                    auto pFeatureRecordMsg = featureRecord->to_protobuf();

                    if (!writer->Write(*pFeatureRecordMsg))
                    {
                        // Client disconnected
                        break;
                    }

                    // Clear the message for reuse
                    pFeatureRecordMsg->Clear();
                }

                return grpc::Status::OK;
            }
            catch (const std::exception &e)
            {
                std::string error = "Internal server error: " + std::string(e.what());
                return grpc::Status(grpc::StatusCode::INTERNAL, error);
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
                return grpc::Status(grpc::StatusCode::INTERNAL, error);
            }
        }

    } // namespace Services
} // namespace OdbDesignServer
