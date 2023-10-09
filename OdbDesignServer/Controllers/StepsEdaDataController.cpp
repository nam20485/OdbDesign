#include "StepsEdaDataController.h"
#include "FileArchive.h"
#include "EdaDataFile.h"


namespace Odb::App::Server
{
	StepsEdaDataController::StepsEdaDataController(OdbDesignServerApp* pServerApp)
		: RouteController(pServerApp)
	{
	}

	void StepsEdaDataController::AddRoutes()
	{
		//
		//	/steps/edadata/package_records?design=sample_design&step=stepName
		//

		// for capturing in lambda below
		//auto pServerApp = m_pServerApp;
		
		// this capture expression works too: 'controller = *this'
		CROW_ROUTE(m_pServerApp->m_crowApp, "/steps/edadata/package_records")
			([pServerApp = this->m_pServerApp](const crow::request& req/*,
											   const crow::response resp*/)
				{				
					auto designName = req.url_params.get("design");
					if (designName && strlen(designName))
					{
						auto stepName = req.url_params.get("step");
						if (stepName && strlen(stepName))
						{
							auto pFileArchive = pServerApp->m_designCache.GetFileArchive(designName);
							if (pFileArchive)
							{
								auto stepsByName = pFileArchive->GetStepsByName();
								auto findIt = stepsByName.find(stepName);
								if (findIt != stepsByName.end())
								{
									auto step = findIt->second;
									auto edaDataFile = step->GetEdaDataFile();									
									//return crow::response(edaDataFile);
									return crow::response(edaDataFile.to_json());
								}
							}
						}
					}

					return crow::response(crow::status::BAD_REQUEST);
				});
	}
}
