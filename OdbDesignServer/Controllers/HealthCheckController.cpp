#include "HealthCheckController.h"

using namespace Odb::Lib::App;

namespace Odb::App::Server
{
	HealthCheckController::HealthCheckController(IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void HealthCheckController::register_routes()
	{
		//register_route_handler("/health_check", std::bind(&HealthCheckController::health_check, this, std::placeholders::_1));
		
		//register_route_handler("/health_check", [&](const crow::request& req) /*-> crow::response*/
		//	{
		//		return crow::response(crow::status::OK, "healthy");
		//	}
		//);

		register_route_handler("/healthz/live", std::bind(&HealthCheckController::health_check_live, this, std::placeholders::_1));
		register_route_handler("/healthz/ready", std::bind(&HealthCheckController::health_check_ready, this, std::placeholders::_1));
		register_route_handler("/healthz/started", std::bind(&HealthCheckController::health_check_started, this, std::placeholders::_1));
	}

	crow::response HealthCheckController::health_check_live(const crow::request& req)
	{
		return crow::response(crow::status::OK, "txt", "healthy: live");
	}

	crow::response HealthCheckController::health_check_ready(const crow::request& req)
	{
		return crow::response(crow::status::OK, "txt", "healthy: ready");
	}

	crow::response HealthCheckController::health_check_started(const crow::request& req)
	{		
		return crow::response(crow::status::OK, "txt", "healthy: started");
	}
}
