#include "HelloWorldController.h"


HelloWorldController::HelloWorldController(crow::SimpleApp& crowApp)
	: RouteController(crowApp)
{
}

void HelloWorldController::AddRoutes()
{
	// /helloworld
	CROW_ROUTE(m_crowApp, "/helloworld")([]() {
		//return "Hello world";
		auto page = crow::mustache::load_text("helloworld.html");
		return page;
		});

	// /hellodesign/<string>
	CROW_ROUTE(m_crowApp, "/hellodesign/<string>")([](std::string designName) {
		auto page = crow::mustache::load("helloworld.html");
		crow::mustache::context ctx({ {"design", designName} });
		return page.render(ctx);
		});
}
