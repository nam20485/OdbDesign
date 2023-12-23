#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"
#include "App/HttpRequest.h"
#include "App/HttpResponse.h"


namespace Odb::App::Server
{
class StepsEdaDataController : public Odb::Lib::App::RouteController
	{
	public:
		StepsEdaDataController(Odb::Lib::App::IOdbServerApp& serverApp);

		void register_routes() override;		

	private:
		crow::response steps_edadata_route_handler(const std::string& designName,
												   const std::string& stepName,
												   const crow::request& req);

		crow::response designs_route_handler(const std::string& designName,
											 const crow::request& req);

		Odb::Lib::App::HttpResponse stepsEdadataHttpRouteHandler(const Odb::Lib::App::HttpRequest& request);

	};
}
