#pragma once

#include "IOdbServerApp.h"
#include "crow_win.h"
#include "DesignCache.h"
#include "Logger.h"
#include "CommandLineArgs.h"
#include "Controllers/RouteController.h"

using namespace Utils;
using namespace Odb::Lib;

namespace Odb::App::Server
{
	class OdbServerAppBase : public IOdbServerApp
	{
	public:
		OdbServerAppBase(int argc, char* argv[]);
		virtual ~OdbServerAppBase();

		static Logger m_logger;

		inline const CommandLineArgs& arguments() const override { return m_commandLineArgs; }
		inline crow::SimpleApp& crow_app() override { return m_crowApp; }
		inline DesignCache& design_cache() override { return m_designCache; }

		virtual Utils::ExitCode Run() override;

	protected:
		DesignCache m_designCache;
		crow::SimpleApp m_crowApp;
		CommandLineArgs m_commandLineArgs;
		RouteController::Vector m_vecControllers;		
		
		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;		

	private:
		void register_routes();

	};
}
