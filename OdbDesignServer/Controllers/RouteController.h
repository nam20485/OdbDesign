#pragma once

#include "../OdbDesignServer.h"
//#include "../OdbDesignServerApp.h"
#include "IOdbServerApp.h"

using namespace Odb::Lib;

namespace Odb::App::Server
{
	class RouteController
	{
	public:		
		RouteController(IOdbServerApp* pServerApp);

		virtual void register_routes() = 0;

		typedef std::vector<std::shared_ptr<RouteController>> Vector;

	protected:		
		IOdbServerApp* m_pServerApp;

		typedef std::function<crow::response(const crow::request& req)> TRouteHandlerFunction;

		void register_route_handler(const std::string& route, TRouteHandlerFunction handler);	
	};
}

