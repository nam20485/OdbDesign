#include "FileModelController.h"
#include <JsonCrowReturnable.h>
#include "UrlEncoding.h"


using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel::Design;
using namespace Utils;

namespace Odb::App::Server
{
	FileModelController::FileModelController(Odb::Lib::App::IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void FileModelController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/steps/<string>/eda_data")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					return this->steps_edadata_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>")
			([&](const crow::request& req, std::string designName)
				{
					return this->designs_route_handler(designName, req);
				});
	}

	crow::response FileModelController::steps_edadata_route_handler(const std::string& designName,
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

	crow::response FileModelController::designs_route_handler(const std::string& designName, const crow::request& req)
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
}