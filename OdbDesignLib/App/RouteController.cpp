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
			([/*&,*/ handler](const crow::request& req)
				{
					//// authenticate request before sending to handler
					//auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					//if (authResp.code != crow::status::OK)
					//{
					//	return authResp;
					//}

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

	crow::response Odb::Lib::App::RouteController::makeLoadedFileModelsResponse() const
	{
		auto unloadedDesignNames = m_serverApp.designs().getUnloadedDesignNames();
		auto loadedFileArchiveNames = m_serverApp.designs().getLoadedFileArchiveNames();
		auto loadedDesignNames = m_serverApp.designs().getLoadedDesignNames();

		crow::json::wvalue::list designs;
		for (const auto& designName : unloadedDesignNames)
		{
			auto loaded = false;
			auto isDesign = false;
			if (std::find(loadedFileArchiveNames.begin(), loadedFileArchiveNames.end(), designName) != loadedFileArchiveNames.end())
			{
				loaded = true;
				isDesign = false;
			}
			else if (std::find(loadedDesignNames.begin(), loadedDesignNames.end(), designName) != loadedDesignNames.end())
			{
				loaded = true;
				isDesign = true;
			}
			crow::json::wvalue design;
			design["name"] = designName;
			design["loaded"] = loaded;
			design["type"] = isDesign? "Design" : "FileArchive";
			designs.push_back(design);
		}
		crow::json::wvalue jsonResponse;
		jsonResponse["filearchives"] = std::move(designs);

#if defined(_DEBUG)
		auto j = jsonResponse.dump();
#endif

		return crow::response(jsonResponse);
	}
}
