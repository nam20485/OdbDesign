#pragma once

#include "IOdbServerApp.h"
#include "../odbdesign_export.h"
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

        crow::response makeLoadedFileModelsResponse() const;

	};
}

