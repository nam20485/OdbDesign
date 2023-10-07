// OdbDesignServer.cpp : Source file for your target.
//

#include "OdbDesignServer.h"
#include "OdbDesign.h"
#include "crow.h"
#include "Controllers/HelloWorldController.h"


void DefineRoutes(crow::SimpleApp& app);

int main()
{
	crow::SimpleApp app;

	//app.loglevel(crow::LogLevel::Debug);	

	// controllers/routes
	HelloWorldController(app).AddRoutes();

	// run the server
	app.port(18080).multithreaded().run();

	return 0;
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
