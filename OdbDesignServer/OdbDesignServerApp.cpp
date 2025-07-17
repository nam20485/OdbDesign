#include <thread>
#include <chrono>
#include <iostream>
#include "OdbDesignServerApp.h"
#include "Controllers/HelloWorldController.h"
#include "Controllers/FileUploadController.h"
#include "Controllers/FileModelController.h"
#include "Controllers/HealthCheckController.h"
#include "Controllers/DesignsController.h"
#include "macros.h"
#include <App/BasicRequestAuthentication.h>

using namespace Odb::Lib::App;

namespace Odb::App::Server
{
	OdbDesignServerApp* OdbDesignServerApp::inst_ = nullptr;
	int heartbeatInterval;
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])
		: OdbServerAppBase(argc, argv)
	{
		inst_ = this;
		heartbeatInterval = args().heartbeatInterval();
		// set last heartbeat time to now
		lastHeartbeat_.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
	}

	//OdbDesignServerApp::~OdbDesignServerApp()
	//{					
	//}

	//Utils::ExitCode OdbDesignServerApp::Run()
	//{
	//	//
	//	// do any initialization here
	//	//		

	//	auto result = OdbServerAppBase::Run();

	//	//
	//	// do any cleanup here
	//	//

	//	return result;
	//}

	void OdbDesignServerApp::add_controllers()
	{
		m_vecControllers.push_back(std::make_shared<HelloWorldController>(*this));		
		m_vecControllers.push_back(std::make_shared<FileUploadController>(*this));
		m_vecControllers.push_back(std::make_shared<FileModelController>(*this));
		m_vecControllers.push_back(std::make_shared<HealthCheckController>(*this));
		m_vecControllers.push_back(std::make_shared<DesignsController>(*this));
	}

	void monitorHeartbeat()
	{
		auto lastTime = OdbDesignServerApp::inst_->lastHeartbeat_.load(std::memory_order_relaxed);
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			auto now = std::chrono::steady_clock::now();
			auto diff = now - lastTime;
			// check heartbeat
			if (diff > std::chrono::seconds(heartbeatInterval))
			{
				std::cerr << "Heartbeat timeout, exiting..." << std::endl;
				exit(0);
			}
			lastTime = OdbDesignServerApp::inst_->lastHeartbeat_.load(std::memory_order_relaxed);
		}
	}

	bool OdbDesignServerApp::preServerRun()
	{
		// CORS
		auto& cors = crow_app().get_middleware<crow::CORSHandler>();
		if (Utils::IsProduction())
		{
			cors.global()
				.headers("*")
				.origin("*");
			//cors.global().methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post);
			//cors.global().origin("73.157.184.219");			
		}
		else
		{
			cors.global()
				.headers("*")
				.origin("*");
			//cors.global().methods(crow::HTTPMethod::Get);
			//cors.global().origin("73.157.184.219");
		}

		// add authentication
		bool disableAuth = args().disableAuthentication();
		auto basicRequestAuth = std::make_unique<BasicRequestAuthentication>(BasicRequestAuthentication(disableAuth));
		request_auth(std::move(basicRequestAuth));

		// start heart beat monitor
		std::thread heartbeatMonitor(monitorHeartbeat);
		heartbeatMonitor.detach(); // run in background

		return true;
	}
}