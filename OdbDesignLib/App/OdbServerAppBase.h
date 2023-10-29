#pragma once

#include "IOdbServerApp.h"
#include "OdbAppBase.h"
#include "RouteController.h"
#include "../crow_win.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT OdbServerAppBase : public OdbAppBase, public IOdbServerApp
	{
	public:
		OdbServerAppBase(int argc, char* argv[]);
		virtual ~OdbServerAppBase();

		crow::SimpleApp& crow_app() override;

		Utils::ExitCode Run() override;		

	protected:
		crow::SimpleApp m_crowApp;
		RouteController::Vector m_vecControllers;

		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;

	private:
		void register_routes();

		static constexpr const char* SSL_CERT_FILE = "ssl.crt";
		static constexpr const char* SSL_KEY_FILE = "ssl.key";
	};
}
