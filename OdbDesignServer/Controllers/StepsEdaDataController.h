#pragma once

#include "RouteController.h"
#include "crow_win.h"
#include "IOdbServerApp.h"

using namespace Odb::Lib;

namespace Odb::App::Server
{
	class StepsEdaDataController : public RouteController
	{
	public:
		StepsEdaDataController(IOdbServerApp* pServerApp);

		void register_routes() override;		

	private:
		crow::response steps_edadata_route_handler(const crow::request& req);

	};
}
