// OdbDesignServer.cpp : Source file for your target.
//

#include "OdbDesignServer.h"
#include "OdbDesign.h"
#include "crow.h"

int main()
{
	crow::SimpleApp app;

	app.loglevel(crow::LogLevel::Info);

	CROW_ROUTE(app, "/")([]() {
		//return "Hello world";
		auto page = crow::mustache::load_text("helloworld.html");
		return page;
		});


	app.port(18080).multithreaded().run();

	return 0;
}
