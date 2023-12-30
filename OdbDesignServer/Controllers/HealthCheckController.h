#pragma once


#include "App/RouteController.h"
#include "App/IOdbServerApp.h"

namespace Odb::App::Server
{

	class HealthCheckController : public Odb::Lib::App::RouteController
	{
	public:
		HealthCheckController(Odb::Lib::App::IOdbServerApp& serverApp);

		virtual void register_routes() override;

	private:
		crow::response health_check(const crow::request& req);

	};
}
