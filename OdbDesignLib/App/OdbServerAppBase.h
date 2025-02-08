#pragma once

#include "IOdbServerApp.h"
#include "OdbAppBase.h"
#include "RouteController.h"
#include "../odbdesign_export.h"
#include "RequestAuthenticationBase.h"
#include <ExitCode.h>
#include <memory>
#include <crow_win.h>

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT OdbServerAppBase : public OdbAppBase, public IOdbServerApp
	{
	public:		
		virtual ~OdbServerAppBase();

		CrowApp& crow_app() override;

		RequestAuthenticationBase& request_auth() override;
		void request_auth(std::unique_ptr<RequestAuthenticationBase> pRequestAuthentication) override;

		Utils::ExitCode Run() override;		

	protected:		
		OdbServerAppBase(int argc, char* argv[]);

		RouteController::Vector m_vecControllers;

		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;

		virtual bool preServerRun();
		virtual bool postServerRun();

	private:
		CrowApp m_crowApp;
		//crow::SimpleApp m_crowApp;
		std::unique_ptr<RequestAuthenticationBase> m_pRequestAuthentication;

		void register_routes();

		static constexpr const char* SSL_CERT_FILE = "ssl.crt";
		static constexpr const char* SSL_KEY_FILE = "ssl.key";
	};
}
