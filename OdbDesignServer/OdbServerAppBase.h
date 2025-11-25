#pragma once

#include <App/IOdbServerApp.h>
#include <App/OdbAppBase.h>
#include <App/RouteController.h>
#include "odbdesign_export.h"
#include <App/RequestAuthenticationBase.h>
#include <App/DesignCache.h>
#include <ExitCode.h>
#include <memory>
#include <crow_win.h>
#include <thread>
#include <atomic>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

namespace Odb::Lib::App
{
	class OdbServerAppBase : public OdbAppBase, public IOdbServerApp
	{
	public:
		~OdbServerAppBase() override;

		CrowApp &crow_app() override;

		RequestAuthenticationBase &request_auth() override;
		void request_auth(std::unique_ptr<RequestAuthenticationBase> pRequestAuthentication) override;

		std::shared_ptr<DesignCache> design_cache() override;
		void design_cache(std::shared_ptr<DesignCache> pDesignCache) override;

		Utils::ExitCode Run() override;

	protected:
		OdbServerAppBase(int argc, char *argv[]);

		RouteController::Vector m_vecControllers;

		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;

		// override in subclasses to implement gRPC service
		virtual void RunGrpcServer(const std::string &server_address, std::shared_ptr<DesignCache> cache);

		virtual bool preServerRun();
		virtual bool postServerRun();

	private:
		static std::atomic<OdbServerAppBase *> s_activeInstance;
		static void HandleSignal(int signum);

		CrowApp m_crowApp;
		std::unique_ptr<RequestAuthenticationBase> m_pRequestAuthentication;
		std::shared_ptr<DesignCache> m_pDesignCache;

		// gRPC server infrastructure
		std::unique_ptr<grpc::Server> m_grpcServer;
		std::unique_ptr<std::thread> m_grpcThread;
		std::atomic<bool> m_shutdownFlag;

		void register_routes();
		void signal_handler(int signum);
		void start_grpc_server();
		void stop_servers();

		static constexpr const char *SSL_CERT_FILE = "ssl.crt";
		static constexpr const char *SSL_KEY_FILE = "ssl.key";
	};
}
