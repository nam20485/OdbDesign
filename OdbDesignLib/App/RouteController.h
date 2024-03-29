#pragma once

#include "IOdbServerApp.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT RouteController
	{
	public:		
		RouteController(IOdbServerApp& serverApp);
		virtual ~RouteController() = default;

		virtual void register_routes() = 0;

		typedef std::vector<std::shared_ptr<RouteController>> Vector;

	protected:		
		IOdbServerApp& m_serverApp;

		typedef std::function<crow::response(const crow::request& req)> TRouteHandlerFunction;

		void register_route_handler(std::string route, TRouteHandlerFunction handler);	
	};
}

