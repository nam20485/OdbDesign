#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"

namespace Odb::App::Server
{
	class FileModelController : public Odb::Lib::App::RouteController
	{
	public:
		FileModelController(Odb::Lib::App::IOdbServerApp& serverApp);
		//virtual ~FileModelController() = default;

		virtual void register_routes() override;

	private:
		crow::response steps_edadata_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response designs_route_handler(const std::string& designName, const crow::request& req);

	};
}
