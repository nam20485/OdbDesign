#include "OdbDesignServerApp.h"
#include "OdbDesign.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/StepsEdaDataController.h"



namespace Odb::App::Server
{
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])
		: m_designCache("designs")
		, m_commandLineArgs(argc, argv)
	{		
	}

	OdbDesignServerApp::~OdbDesignServerApp()
	{		
		//m_designCache.Clear();
	}

	Utils::ExitCode OdbDesignServerApp::Run()
	{
		//m_crowApp.loglevel(crow::LogLevel::Debug);	

		// controller routes
		register_routes();

		// run the server
		m_crowApp.port(18080).multithreaded().run();

		return Utils::ExitCode::Success;
	}

	void OdbDesignServerApp::register_routes()
	{
		HelloWorldController helloWorld(this);
		helloWorld.register_routes();

		StepsEdaDataController edaData(this);
		edaData.register_routes();
	}
}