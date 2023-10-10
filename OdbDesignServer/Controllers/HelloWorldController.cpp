#include "HelloWorldController.h"


namespace Odb::App::Server
{
	HelloWorldController::HelloWorldController(OdbDesignServerApp* pServerApp)
		: RouteController(pServerApp)
	{
	}

	void HelloWorldController::register_routes()
	{
		// /helloworld
		CROW_ROUTE(m_pServerApp->crow_app(), "/helloworld")([]() {
			//return "Hello world";
			auto page = crow::mustache::load_text("helloworld.html");
			return page;
			});

		// /hellodesign/<string>
		CROW_ROUTE(m_pServerApp->crow_app(), "/hellodesign/<string>")([](std::string designName) {
			auto page = crow::mustache::load("helloworld.html");
			crow::mustache::context ctx({ {"design", designName} });
			return page.render(ctx);
			});
	}
}