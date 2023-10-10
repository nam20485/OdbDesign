#include "RouteController.h"


namespace Odb::App::Server
{
	RouteController::RouteController(OdbDesignServerApp* pServerApp)
		: m_pServerApp(pServerApp)
	{
	}

	void RouteController::register_route_handler(const std::string& route, TRouteHandlerFunction handler)
	{		
		CROW_ROUTE(m_pServerApp->get_crow_app(), "/steps/edadata/package_records")
			([&](const crow::request& req)
				{
					return handler(req);
				});
	}
}
