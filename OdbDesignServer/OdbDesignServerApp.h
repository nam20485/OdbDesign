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

		Utils::Logger m_logger;
		Odb::Lib::DesignCache m_designCache;
		crow::SimpleApp m_crowApp;
		Utils::CommandLineArgs m_commandLineArgs;

		Utils::ExitCode Run();

	private:				
		void AddRoutes();

	};
}
