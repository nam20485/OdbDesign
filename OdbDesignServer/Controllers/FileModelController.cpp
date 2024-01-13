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

	//StepDirectory::StringMap m_stepsByName;
	//SymbolsDirectory::StringMap m_symbolsDirectoriesByName;
	
	////MiscInfoFile m_miscInfoFile;
	////MatrixFile m_matrixFile;
	////StandardFontsFile m_standardFontsFile;	
	////AttrListFile m_miscAttrListFile;

	void FileModelController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>")
			([&](const crow::request& req, std::string designName)
				{
					return this->designs_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/steps/<string>")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					return this->steps_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/steps/<string>/eda_data")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					return this->steps_edadata_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/symbols/<string>")
			([&](const crow::request& req, std::string designName, std::string symbolName)
				{
					return this->symbols_route_handler(designName, symbolName, req);
				});

		//register_route_handler("/filemodel/misc/attrlist", std::bind(&FileModelController::misc_attrlist_route_handler, this, std::placeholders::_1));
		//register_route_handler("/filemodel/matrix/matrix", std::bind(&FileModelController::matrix_matrix_route_handler, this, std::placeholders::_1));
		//register_route_handler("/filemodel/misc/info", std::bind(&FileModelController::misc_info_route_handler, this, std::placeholders::_1));

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/misc/attrlist")
			([&](const crow::request& req, std::string designName)
				{
					return this->misc_attrlist_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/matrix/matrix")
			([&](const crow::request& req, std::string designName)
				{
					return this->matrix_matrix_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/misc/info")
			([&](const crow::request& req, std::string designName)
				{
					return this->misc_info_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodel/<string>/fonts/standard")
			([&](const crow::request& req, std::string designName)
				{
					return this->fonts_standard_route_handler(designName, req);
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

	crow::response FileModelController::steps_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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
		return crow::response(JsonCrowReturnable(*step));
	}

	crow::response FileModelController::symbols_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto symbolNameDecoded = UrlEncoding::decode(symbolName);
		if (symbolNameDecoded.empty())
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

		auto& symbolsByName = pFileArchive->GetSymbolsDirectoriesByName();
		auto findIt = symbolsByName.find(symbolNameDecoded);
		if (findIt == symbolsByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " symbol: \"" << symbolNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		auto& symbol = findIt->second;		
		return crow::response(JsonCrowReturnable(*symbol));
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

	crow::response FileModelController::misc_attrlist_route_handler(const std::string& designName, const crow::request& req)
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

		auto& miscAttrListFile = pFileArchive->GetMiscAttrListFile();
		return crow::response(JsonCrowReturnable(miscAttrListFile));
	}

	crow::response FileModelController::matrix_matrix_route_handler(const std::string& designName, const crow::request& req)
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

		auto& matrixFile = pFileArchive->GetMatrixFile();
		return crow::response(JsonCrowReturnable(matrixFile));
	}

	crow::response FileModelController::misc_info_route_handler(const std::string& designName, const crow::request& req)
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

		auto& miscInfoFile = pFileArchive->GetMiscInfoFile();
		return crow::response(JsonCrowReturnable(miscInfoFile));
	}

	crow::response FileModelController::fonts_standard_route_handler(const std::string& designName, const crow::request& req)
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

		auto& standardFontsFile = pFileArchive->GetStandardFontsFile();
		return crow::response(JsonCrowReturnable(standardFontsFile));
	}
}