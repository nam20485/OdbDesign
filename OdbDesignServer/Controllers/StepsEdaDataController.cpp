#include "StepsEdaDataController.h"
#include "FileArchive.h"

namespace Odb::App::Server
{

	StepsEdaDataController::StepsEdaDataController(crow::SimpleApp& crowApp)
		: RouteController(crowApp)
	{
	}

	void StepsEdaDataController::AddRoutes()
	{
		//
		//	/steps/edadata/package_records
		//
		CROW_ROUTE(m_crowApp, "/steps/edadata/package_records")
			([](const crow::request& req/*,
				const crow::response resp*/) {

				// /steps/edadata/package_records?design=sample_design

					auto designName = req.url_params.get("design");
					if (designName)
					{
						Odb::Lib::FileModel::Design::FileArchive odbDesign(designName);
						auto success = odbDesign.ParseFileModel();
						if (!success)
						{
							//return crow::response(crow::status::BAD_REQUEST);
							//resp.end();
						}

						//auto packageRecords = odbDesign.GetPackageRecords();
						//packageRecords.				
					}

					return crow::response(crow::status::BAD_REQUEST);

				});
	}
}
