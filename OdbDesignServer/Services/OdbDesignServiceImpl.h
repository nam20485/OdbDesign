#pragma once

#include "grpc/service.grpc.pb.h"
#include "App/DesignCache.h"
#include <memory>
#include <design.pb.h>
#include <featuresfile.pb.h>
#include <grpc/service.pb.h>
#include <grpcpp/impl/status.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/sync_stream.h>


namespace OdbDesignServer {
    namespace Services {

        class OdbDesignServiceImpl final : public Odb::Grpc::OdbDesignService::Service {
        public:
            explicit OdbDesignServiceImpl(std::shared_ptr<Odb::Lib::App::DesignCache> cache);

            grpc::Status GetDesign(grpc::ServerContext* context,
                const Odb::Grpc::GetDesignRequest* request,
                Odb::Lib::Protobuf::ProductModel::Design* response) override;

            grpc::Status GetLayerFeaturesStream(grpc::ServerContext* context,
                const Odb::Grpc::GetLayerFeaturesRequest* request,
                grpc::ServerWriter<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>* writer) override;

            grpc::Status HealthCheck(grpc::ServerContext* context,
                const Odb::Grpc::HealthCheckRequest* request,
                Odb::Grpc::HealthCheckResponse* response) override;
        private:
            std::shared_ptr<Odb::Lib::App::DesignCache> m_designCache;
        };

    } // namespace Services
} // namespace OdbDesignServer