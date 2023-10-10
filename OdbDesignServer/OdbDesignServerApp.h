#pragma once

#include "OdbDesignServer.h"
#include "crow.h"
#include "ExitCode.h"
#include "DesignCache.h"
#include <vector>
#include "Logger.h"
#include "CommandLineArgs.h"


namespace Odb::App::Server
{
	class OdbDesignServerApp
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		~OdbDesignServerApp();

		static Utils::Logger m_logger;

		inline const Utils::CommandLineArgs& get_args() const { return m_commandLineArgs; }
		inline crow::SimpleApp& get_crow_app() { return m_crowApp; }
		inline Odb::Lib::DesignCache& get_design_cache() { return m_designCache; }
				
		Utils::ExitCode Run();		

	private:				
		Odb::Lib::DesignCache m_designCache;
		crow::SimpleApp m_crowApp;
		Utils::CommandLineArgs m_commandLineArgs;

		void register_routes();

	};
}
