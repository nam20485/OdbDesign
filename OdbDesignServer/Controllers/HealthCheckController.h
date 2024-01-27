#pragma once


#include "App/RouteController.h"
#include "App/IOdbServerApp.h"

namespace Odb::App::Server
{

	class HealthCheckController : public Odb::Lib::App::RouteController
	{
	public:
		HealthCheckController(Odb::Lib::App::IOdbServerApp& serverApp);

		void register_routes() override;

	private:
		crow::response health_check_live(const crow::request& req);
		crow::response health_check_ready(const crow::request& req);
		crow::response health_check_started(const crow::request& req);

	};
}
