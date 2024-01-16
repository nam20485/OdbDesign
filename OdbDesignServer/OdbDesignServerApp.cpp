#include "OdbDesignServerApp.h"
#include "OdbDesign.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/FileUploadController.h"
#include "Controllers/FileModelController.h"
#include "Controllers/HealthCheckController.h"
#include "macros.h"


namespace Odb::App::Server
{
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])
		: OdbServerAppBase(argc, argv)
	{		
	}

	//OdbDesignServerApp::~OdbDesignServerApp()
	//{					
	//}

	Utils::ExitCode OdbDesignServerApp::Run()
	{
		//
		// do any initialization here
		//

		// CORS
		auto& cors = crow_app().get_middleware<crow::CORSHandler>();
		if (Utils::IsProduction())
		{
			cors.global().headers("*");
			//cors.global().methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post);
			cors.global().origin("*");
			//cors.global().origin("73.157.184.219");			
		}
		else
		{
			cors.global().headers("*");
			//cors.global().methods(crow::HTTPMethod::Get);
			cors.global().origin("*");
			//cors.global().origin("73.157.184.219");
		}

		return OdbServerAppBase::Run();

		//
		// do any cleanup here
		//
	}

	void OdbDesignServerApp::add_controllers()
	{
		m_vecControllers.push_back(std::make_shared<HelloWorldController>(*this));		
		m_vecControllers.push_back(std::make_shared<FileUploadController>(*this));
		m_vecControllers.push_back(std::make_shared<FileModelController>(*this));
		m_vecControllers.push_back(std::make_shared<HealthCheckController>(*this));
	}	
}