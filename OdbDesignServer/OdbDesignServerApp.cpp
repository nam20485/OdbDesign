#include "OdbDesignServerApp.h"
#include "OdbDesign.h"
#include "Controllers/HelloWorldController.h"



namespace Odb::App::Server
{
	OdbDesignServerApp::OdbDesignServerApp(int argc, char* argv[])		
	{
		// save arguments in std::string vector
		for (int i = 0; i < argc; i++)
		{
			m_vecArgv.push_back(argv[i]);
		}
	}

	OdbDesignServerApp::~OdbDesignServerApp()
	{
		m_vecArgv.clear();
	}

	ExitCode OdbDesignServerApp::Run()
	{
		//m_crowApp.loglevel(crow::LogLevel::Debug);	

		// controllers/routes
		HelloWorldController(m_crowApp).AddRoutes();

		// run the server
		m_crowApp.port(18080).multithreaded().run();

		return ExitCode::Success;
	}

	void DefineRoutes(crow::SimpleApp& app)
	{
		//app.loglevel(crow::LogLevel::Info);	

		//CROW_ROUTE(app, "/helloworld")([]() {
		//	//return "Hello world";
		//	auto page = crow::mustache::load_text("helloworld.html");
		//	return page;
		//	});

		//CROW_ROUTE(app, "/hellodesign/<string>")([](std::string designName) {
		//	auto page = crow::mustache::load("helloworld.html");
		//	crow::mustache::context ctx({ {"design", designName} });
		//	return page.render(ctx);
		//	});

		CROW_ROUTE(app, "/steps/edadata/package_records")([]() {

			// /steps/edadata/package_records?design=sample_design

			auto page = crow::mustache::load_text("helloworld.html");
			return page;
			});
	}
}