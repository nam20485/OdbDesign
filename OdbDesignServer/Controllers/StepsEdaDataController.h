#pragma once

#include "RouteController.h"
#include "crow_win.h"
#include "IOdbServerApp.h"


namespace Odb::App::Server
{
	class StepsEdaDataController : public Odb::Lib::RouteController
	{
	public:
		StepsEdaDataController(Odb::Lib::IOdbServerApp& serverApp);

		void register_routes() override;		

	private:
		crow::response steps_edadata_route_handler(const std::string& designName,
												   const std::string& stepName,
												   const crow::request& req);

	};
}
