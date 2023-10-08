#include "StepsEdaDataController.h"
#include "FileArchive.h"


namespace Odb::App::Server
{
	StepsEdaDataController::StepsEdaDataController(OdbDesignServerApp* pServerApp)
		: RouteController(pServerApp)
	{
	}

	void StepsEdaDataController::AddRoutes()
	{
		//
		//	/steps/edadata/package_records?design=sample_design
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
						auto pFileArchive = pServerApp->m_designCache.GetFileArchive(designName);
						if (pFileArchive)
						{
							auto productName = pFileArchive->GetProductName();
							return crow::response(crow::status::OK, productName);

							//	auto page = crow::mustache::load_text("helloworld.html");
							//	return page;
						}
					}

					return crow::response(crow::status::BAD_REQUEST);
				});
	}
}
