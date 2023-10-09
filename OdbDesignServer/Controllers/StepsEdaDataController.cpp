#include "StepsEdaDataController.h"
#include "FileArchive.h"
#include "EdaDataFile.h"
#include <format>


namespace Odb::App::Server
{
	StepsEdaDataController::StepsEdaDataController(OdbDesignServerApp* pServerApp)
		: RouteController(pServerApp)
	{
	}

	void StepsEdaDataController::register_routes()
	{
		//
		//	/steps/edadata/package_records?design=sample_design&step=stepName
		//

		// TODO: figure out why capture here is wierd (i.e. how to capture pServerApp so it can be used in the member fxn handler)
		CROW_ROUTE(m_pServerApp->m_crowApp, "/steps/edadata/package_records")
			([&, pServerApp = this->m_pServerApp](const crow::request& req)
				{
					return this->steps_edadata_route_handler(req, pServerApp);
				});

		//register_route_handler("/steps/edadata/package_records", std::bind(&StepsEdaDataController::steps_edadata_route_handler, this, std::placeholders::_1));			
		/*[&](const crow::request& req)
		{
			return steps_edadata_route_handler(req);
		});*/
	}	

	crow::response StepsEdaDataController::steps_edadata_route_handler(const crow::request& req, OdbDesignServerApp* pServerApp)
	{
		auto designName = req.url_params.get("design");
		if (designName == nullptr || strlen(designName) == 0)
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto stepName = req.url_params.get("step");
		if (stepName == nullptr || strlen(stepName) == 0)
		{
			return crow::response(crow::status::BAD_REQUEST, "step name not specified");
		}

		auto pFileArchive = pServerApp->m_designCache.GetFileArchive(designName);
		if (pFileArchive == nullptr)
		{
			return crow::response(crow::status::BAD_REQUEST, std::format("design: \"{0}\" not found", designName));
		}

		auto stepsByName = pFileArchive->GetStepsByName();
		auto findIt = stepsByName.find(stepName);
		if (findIt == stepsByName.end())
		{
			return crow::response(crow::status::BAD_REQUEST, std::format("step: \"{0}\" not found", stepName));
		}

		auto step = findIt->second;
		auto edaDataFile = step->GetEdaDataFile();
		return crow::response(edaDataFile);
	}
}
