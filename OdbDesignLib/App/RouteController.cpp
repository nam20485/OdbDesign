#include "RouteController.h"


namespace Odb::Lib::App
{
	RouteController::RouteController(IOdbServerApp& serverApp)
		: m_serverApp(serverApp)
	{
	}

	void RouteController::register_route_handler(const std::string& route, TRouteHandlerFunction handler)
	{				
		////.template register_handler<crow::black_magic::crow_internal::get_parameter_tag()>(m_pServerApp->crow_app(), handler);
		//CROW_ROUTE(m_pServerApp->crow_app(), "/steps/edadata")
		//	([&](const crow::request& req)
		//		{
		//			return handler(req);
		//		});
	}
}
