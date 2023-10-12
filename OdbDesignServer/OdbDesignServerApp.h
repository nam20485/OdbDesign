#pragma once

#include "OdbDesignServer.h"
#include "crow_win.h"
#include "ExitCode.h"
#include "DesignCache.h"
#include <vector>
#include "Logger.h"
#include "CommandLineArgs.h"

using namespace Utils;
using namespace Odb::Lib;

namespace Odb::App::Server
{
	class OdbDesignServerApp
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		~OdbDesignServerApp();

		static Logger m_logger;

		inline const CommandLineArgs& arguments() const { return m_commandLineArgs; }
		inline crow::SimpleApp& crow_app() { return m_crowApp; }
		inline DesignCache& design_cache() { return m_designCache; }
				
		ExitCode Run();		

	private:				
		DesignCache m_designCache;
		crow::SimpleApp m_crowApp;
		CommandLineArgs m_commandLineArgs;

		void register_routes();

	};
}
