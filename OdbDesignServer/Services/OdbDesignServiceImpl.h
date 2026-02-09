#pragma once

#include "odbdesign_export.h"
#include "service.grpc.pb.h"
#include "App/DesignCache.h"
#include "../Config/GrpcServiceConfig.h"
#include <memory>
#include <design.pb.h>
#include <featuresfile.pb.h>
#include <service.pb.h>
#include <symbolname.pb.h>
#include <grpcpp/impl/status.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/sync_stream.h>
#include <google/protobuf/arena.h>


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

            grpc::Status HealthCheck(grpc::ServerContext* context,
                const Odb::Grpc::HealthCheckRequest* request,
                Odb::Grpc::HealthCheckResponse* response) override;
        private:
            std::shared_ptr<Odb::Lib::App::DesignCache> m_designCache;
            std::shared_ptr<Config::GrpcServiceConfig> m_config;
        };

    } // namespace Services
} // namespace OdbDesignServer
