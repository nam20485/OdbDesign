#pragma once

#include "OdbDesignServer.h"
#include "crow.h"
#include "ExitCode.h"
#include "DesignCache.h"
#include <vector>


namespace Odb::App::Server
{
	class OdbDesignServerApp
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		~OdbDesignServerApp();

		Utils::ExitCode Run();

	private:
		crow::SimpleApp m_crowApp;
		Odb::Lib::DesignCache m_designCache;

		std::vector<std::string> m_vecArgv;

	};
}
