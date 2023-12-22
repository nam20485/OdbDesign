#include "OdbDesignServerApp.h"
#include "OdbDesign.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/StepsEdaDataController.h"
#include "Controllers/FileUploadController.h"

using namespace Odb::Lib::App;

namespace Odb::App::Server
{
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])
		: OdbServerAppBase(argc, argv)
	{		
	}

	OdbDesignServerApp::~OdbDesignServerApp()
	{					
	}

	//Utils::ExitCode OdbDesignServerApp::Run()
	//{
	//	//
	//	// do any initialization here
	//	//

	//	return OdbServerAppBase::Run();

	//	//
	//	// do any cleanup here
	//	//
	//}

	void OdbDesignServerApp::add_controllers()
	{
		//RouteController::Vector controllers;
		RouteController::Vector& controllers = m_vecControllers;
		controllers.push_back(std::make_shared<HelloWorldController>(*this));
		controllers.push_back(std::make_shared<StepsEdaDataController>(*this));
		controllers.push_back(std::make_shared<FileUploadController>(*this));
		//OdbServerAppBase::add_controllers(controllers);
	}	
}