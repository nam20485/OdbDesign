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
		
		register_route_handler("/health_check", [&](const crow::request& req) /*-> crow::response*/
			{
				return crow::response(crow::status::OK, "healthy");
			}
		);
	}

	crow::response HealthCheckController::health_check(const crow::request& req)
	{
		return crow::response(crow::status::OK, "healthy");
	}
}
