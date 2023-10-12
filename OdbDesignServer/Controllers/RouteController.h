#pragma once

#include "../OdbDesignServer.h"
#include "crow.h"
//#include "../OdbDesignServerApp.h"


namespace Odb::App::Server
{
	// forward declaration
	class OdbDesignServerApp;

	class RouteController
	{
	public:		
		RouteController(OdbDesignServerApp* pServerApp);

		virtual void register_routes() = 0;

		typedef std::vector<std::shared_ptr<RouteController>> Vector;

	protected:		
		OdbDesignServerApp* m_pServerApp;

		typedef std::function<crow::response(const crow::request& req)> TRouteHandlerFunction;

		void register_route_handler(const std::string& route, TRouteHandlerFunction handler);	
	};
}

