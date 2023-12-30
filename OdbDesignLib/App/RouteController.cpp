#include "RouteController.h"


namespace Odb::Lib::App
{
	RouteController::RouteController(IOdbServerApp& serverApp)
		: m_serverApp(serverApp)
	{
	}

	void RouteController::register_route_handler(std::string route, TRouteHandlerFunction handler)
	{				
		m_serverApp.crow_app().route_dynamic(std::move(route))
			([handler](const crow::request& req)
				{
					return handler(req);
				});

		//app.route<crow::black_magick::get_parameter_tag(url)>(url)
		// or
		//app.route_dynamic(url)

		//m_serverApp.crow_app().route_dynamic(const_cast<std::string&&>(route)).methods("GET"_method, "POST"_method)([&](const crow::request& req)
		//	{
		//		return handler(req);
		//	})

		////.template register_handler<crow::black_magic::crow_internal::get_parameter_tag()>(m_pServerApp->crow_app(), handler);
		//CROW_ROUTE(m_pServerApp->crow_app(), "/steps/edadata")
		//	([&](const crow::request& req)
		//		{
		//			return handler(req);
		//		});
	}
}
