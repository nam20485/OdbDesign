#pragma once

#include "odbdesign_export.h"
#include "service.grpc.pb.h"
#include "App/DesignCache.h"
#include "../Config/GrpcServiceConfig.h"
#include <memory>
#include <design.pb.h>
#include <featuresfile.pb.h>
#include <service.pb.h>
#include <standardfontsfile.pb.h>
#include <symbolname.pb.h>
#include <grpcpp/impl/status.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/sync_stream.h>


namespace OdbDesignServer {
    namespace Services {

        class OdbDesignServiceImpl final : public Odb::Grpc::OdbDesignService::Service {
        public:
            explicit OdbDesignServiceImpl(std::shared_ptr<Odb::Lib::App::DesignCache> cache,
                std::shared_ptr<Config::GrpcServiceConfig> config = nullptr);

            grpc::Status GetDesign(grpc::ServerContext* context,
                const Odb::Grpc::GetDesignRequest* request,
                Odb::Lib::Protobuf::ProductModel::Design* response) override;

            grpc::Status GetLayerFeaturesStream(grpc::ServerContext* context,
                const Odb::Grpc::GetLayerFeaturesRequest* request,
                grpc::ServerWriter<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>* writer) override;

            grpc::Status GetLayerFeaturesBatchStream(grpc::ServerContext* context,
                const Odb::Grpc::GetLayerFeaturesRequest* request,
                grpc::ServerWriter<Odb::Grpc::FeatureRecordBatch>* writer) override;

            grpc::Status GetLayerSymbols(grpc::ServerContext* context,
                const Odb::Grpc::GetLayerSymbolsRequest* request,
                Odb::Grpc::GetLayerSymbolsResponse* response) override;

            grpc::Status GetStandardFonts(grpc::ServerContext* context,
                const Odb::Grpc::GetStandardFontsRequest* request,
                Odb::Lib::Protobuf::StandardFontsFile* response) override;

            grpc::Status RequestLoadDesign(grpc::ServerContext* context,
                const Odb::Grpc::RequestLoadDesignRequest* request,
                Odb::Grpc::RequestLoadDesignResponse* response) override;

            grpc::Status HealthCheck(grpc::ServerContext* context,
                const Odb::Grpc::HealthCheckRequest* request,
                Odb::Grpc::HealthCheckResponse* response) override;
        private:
            std::shared_ptr<Odb::Lib::App::DesignCache> m_designCache;
            std::shared_ptr<Config::GrpcServiceConfig> m_config;
        };

        // Shared status mapping behind RequestLoadDesign (gRPC) and its REST twin
        // (POST /designs/<name>/load). Pure lookup: never blocks on a parse; the
        // only mutating call is the LoadDesignAsync kick, which returns
        // immediately. Failed falls through to the kick path because a failed
        // load is retryable by design.
        inline Odb::Grpc::LoadStatus ComputeRequestLoadStatus(
            Odb::Lib::App::DesignCache& designCache, const std::string& designName)
        {
            using Odb::Lib::App::DesignCache;

            switch (designCache.GetLoadState(designName))
            {
            case DesignCache::LoadState::Loading:
                return Odb::Grpc::LOAD_ALREADY_LOADING;
            case DesignCache::LoadState::Loaded:
                return Odb::Grpc::LOAD_ALREADY_LOADED;
            default:
                break;
            }

            if (!designCache.ContainsDesign(designName))
            {
                return Odb::Grpc::LOAD_NOT_FOUND;
            }
            return designCache.LoadDesignAsync(designName)
                ? Odb::Grpc::LOAD_ACCEPTED
                : Odb::Grpc::LOAD_ALREADY_LOADING;
        }

        // JSON status strings for the REST twin's response body
        // ("accepted" | "already_loading" | "already_loaded" | "not_found").
        // If/else rather than a switch: protobuf's synthetic
        // _INT_MIN/MAX_SENTINEL_DO_NOT_USE_ enum values would trip -Wswitch.
        inline const char* LoadStatusToString(Odb::Grpc::LoadStatus status)
        {
            if (status == Odb::Grpc::LOAD_ACCEPTED) return "accepted";
            if (status == Odb::Grpc::LOAD_ALREADY_LOADING) return "already_loading";
            if (status == Odb::Grpc::LOAD_ALREADY_LOADED) return "already_loaded";
            if (status == Odb::Grpc::LOAD_NOT_FOUND) return "not_found";
            return "unknown";
        }

    } // namespace Services
} // namespace OdbDesignServer
