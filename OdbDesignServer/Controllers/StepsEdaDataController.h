#pragma once

#include "RouteController.h"
#include "crow_win.h"


namespace Odb::App::Server
{
	class StepsEdaDataController : public RouteController
	{
	public:
		StepsEdaDataController(OdbDesignServerApp* pServerApp);

		void register_routes() override;		

	private:
		crow::response steps_edadata_route_handler(const crow::request& req, OdbDesignServerApp* pServerApp);

	};
}
