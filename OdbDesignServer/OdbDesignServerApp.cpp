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
	}

	Utils::ExitCode OdbDesignServerApp::Run()
	{
		//m_crowApp.loglevel(crow::LogLevel::Debug)
		//m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		// controller routes
		register_routes();

		// run the server
		m_crowApp.port(18080).multithreaded().run();

		return Utils::ExitCode::Success;
	}

	void OdbDesignServerApp::register_routes()
	{
		HelloWorldController* pHelloWorldRoutes = new HelloWorldController(this);
		pHelloWorldRoutes->register_routes();

		StepsEdaDataController* pStepsEdaDataRoutes = new StepsEdaDataController(this);
		pStepsEdaDataRoutes->register_routes();
	}
}