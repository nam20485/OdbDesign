#pragma once

#include "IOdbServerApp.h"
#include "OdbAppBase.h"
#include "RouteController.h"
#include "../odbdesign_export.h"
#include "IHttpServer.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT OdbServerAppBase : public OdbAppBase, public IOdbServerApp
	{
	public:
		OdbServerAppBase(int argc, char* argv[]);
		virtual ~OdbServerAppBase();

		//crow::SimpleApp& crow_app() override;
		IHttpServer& http_server() override;

		Utils::ExitCode Run() override;		

	protected:
		RouteController::Vector m_vecControllers;

		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;
		//void add_controllers(const RouteController::Vector& controllers);

	private:
		//crow::SimpleApp m_crowApp;
		std::unique_ptr<IHttpServer> m_pHttpServer;

		void register_routes();

		static constexpr const char* SSL_CERT_FILE = "ssl.crt";
		static constexpr const char* SSL_KEY_FILE = "ssl.key";
	};
}
