#include "StepsEdaDataController.h"
#include "FileModel/Design/FileArchive.h"
#include "FileModel/Design/EdaDataFile.h"
#include "JsonCrowReturnable.h"
#include <sstream>
#include "UrlEncoding.h"


using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel::Design;
using namespace Utils;

namespace Odb::App::Server
{
	StepsEdaDataController::StepsEdaDataController(IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void StepsEdaDataController::register_routes()
	{		
		m_serverApp.http_server().addRoute("/designs/<string>/steps/<string>/eda_data",
											IHttpServer::HttpMethod::Get,
											std::bind(&StepsEdaDataController::stepsEdadataHttpRouteHandler, this, std::placeholders::_1));

		////
		////	/steps/edadata?design=sample_design&step=stepName
		////		
		//// TODO: figure out why capture here is weird (i.e. how to capture pServerApp so it can be used in the member fxn handler)
		//CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/steps/<string>/eda_data")
		//	([&](const crow::request& req, std::string designName, std::string stepName)
		//		{
		//			return this->steps_edadata_route_handler(designName, stepName, req);
		//		});

		//CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>")
		//	([&](const crow::request& req, std::string designName)
		//		{
		//			return this->designs_route_handler(designName, req);
		//		});

		//app.route_dynamic(url)

		//register_route_handler("/steps/edadata/package_records", std::bind(&StepsEdaDataController::steps_edadata_route_handler, this, std::placeholders::_1));			
		/*[&](const crow::request& req)
		{
			return steps_edadata_route_handler(req);
		});*/
	}	



	crow::response StepsEdaDataController::steps_edadata_route_handler(const std::string& designName, 
																	   const std::string& stepName,
																	   const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto stepNameDecoded = UrlEncoding::decode(stepName);
		if (stepNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "step name not specified");
		}
	
		auto pFileArchive = m_serverApp.designs().GetFileArchive(designNameDecoded);
		if (pFileArchive == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		auto& stepsByName = pFileArchive->GetStepsByName();
		auto findIt = stepsByName.find(stepNameDecoded);
		if (findIt == stepsByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		auto& step = findIt->second;
		auto& edaDataFile = step->GetEdaDataFile();
		return crow::response(JsonCrowReturnable(edaDataFile));
	}

	crow::response StepsEdaDataController::designs_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto pFileArchive = m_serverApp.designs().GetFileArchive(designNameDecoded);
		if (pFileArchive == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		return crow::response(JsonCrowReturnable(*pFileArchive));
	}

	Odb::Lib::App::HttpResponse StepsEdaDataController::stepsEdadataHttpRouteHandler(const Odb::Lib::App::HttpRequest& request)
	{


		return Odb::Lib::App::HttpResponse();
	}
}
