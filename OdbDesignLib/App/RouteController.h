#pragma once

#include "IOdbServerApp.h"
#include "../odbdesign_export.h"
#include <optional>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT RouteController
	{
	public:		
		virtual ~RouteController() = default;

		virtual void register_routes() = 0;

		typedef std::vector<std::shared_ptr<RouteController>> Vector;

	protected:		
		IOdbServerApp& m_serverApp;

		RouteController(IOdbServerApp& serverApp);

		typedef std::function<crow::response(const crow::request& req)> TRouteHandlerFunction;

		void register_route_handler(std::string route, TRouteHandlerFunction handler);	

        crow::response makeLoadedFileModelsResponse(bool) const;

		// ---- HTTP caching for per-design data GETs (M1.3) ----

		// Conditional-GET support. Computes the design's strong ETag (see
		// Utils/ETag.h: design name + archive mtime/size + endpoint path) BEFORE
		// the archive is loaded or anything serialized; if the request's
		// If-None-Match matches it, returns the 304 response (empty body, no
		// serialization work — that is the point). Returns std::nullopt when the
		// caller must proceed, with etag carrying the computed tag (empty when no
		// archive file exists — the load will 404 and no caching headers go out).
		// Call AFTER authentication and BEFORE any design loading.
		std::optional<crow::response> checkConditionalGet(const crow::request& req,
			const std::string& designName,
			const std::string& endpointPath,
			std::string& etag) const;

		// Attaches ETag + Cache-Control to a successful data response. No-op when
		// etag is empty (archive file not found/statable).
		crow::response withCacheHeaders(crow::response response, const std::string& etag) const;

		constexpr inline static const char* CACHE_CONTROL_DATA_VALUE = "private, max-age=3600";

	};
}

