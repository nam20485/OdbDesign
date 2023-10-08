#pragma once

#include "../OdbDesignServer.h"
#include "crow.h"
#include "../OdbDesignServerApp.h"


namespace Odb::App::Server
{
	class RouteController
	{
	public:		
		RouteController(OdbDesignServerApp* pServerApp);

		virtual void AddRoutes() = 0;

	protected:		
		OdbDesignServerApp* m_pServerApp;

	};
}

