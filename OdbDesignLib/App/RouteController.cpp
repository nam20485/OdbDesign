#include "RouteController.h"
#include <ETag.h>


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

	crow::response Odb::Lib::App::RouteController::makeLoadedFileModelsResponse(bool created) const
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

		// determine type of success code
		auto httpCode = crow::status::OK;
		if (created)
		{
			httpCode = crow::status::CREATED;
		}
		else if (unloadedDesignNames.empty())
		{
			httpCode = crow::status::NO_CONTENT;
		}

		return crow::response(httpCode , jsonResponse);
	}

	std::optional<crow::response> RouteController::checkConditionalGet(const crow::request& req,
		const std::string& designName,
		const std::string& endpointPath,
		std::string& etag) const
	{
		// Computed from the archive file on disk, not the (possibly not yet
		// loaded) cached object, so the 304 decision never pays for a load or a
		// serialization. A stat-to-load race (archive replaced in between) is
		// benign: the 200 body then carries the pre-load tag, the next request
		// recomputes the post-replacement tag, and the client's stale copy is
		// revalidated and refreshed.
		etag = Utils::MakeDesignEtag(m_serverApp.args().designsDir(), designName, endpointPath);
		if (etag.empty())
		{
			return std::nullopt;
		}

		const auto& ifNoneMatch = req.get_header_value("If-None-Match");
		if (!ifNoneMatch.empty() && Utils::IfNoneMatchMatches(ifNoneMatch, etag))
		{
			// 304 short-circuits the handler: no archive load, no to_json().
			// ETag + Cache-Control are echoed per RFC 7232 so the client can
			// refresh its cached header set.
			crow::response notModified(crow::status::NOT_MODIFIED);
			notModified.set_header("ETag", etag);
			notModified.set_header("Cache-Control", CACHE_CONTROL_DATA_VALUE);
			return notModified;
		}

		return std::nullopt;
	}

	crow::response RouteController::withCacheHeaders(crow::response response, const std::string& etag) const
	{
		// private: design payloads are user-scoped data and must not be served
		// from shared/proxy caches; max-age allows short-term client reuse
		// between revalidations.
		if (!etag.empty())
		{
			response.set_header("ETag", etag);
			response.set_header("Cache-Control", CACHE_CONTROL_DATA_VALUE);
		}
		return response;
	}
}
