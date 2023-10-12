#include "RouteController.h"
#include "crow_win.h"


namespace Odb::Lib
{
	RouteController::RouteController(IOdbServerApp* pServerApp)
		: m_pServerApp(pServerApp)
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
