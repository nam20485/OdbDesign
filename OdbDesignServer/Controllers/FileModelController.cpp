#include "FileModelController.h"
#include <JsonCrowReturnable.h>
#include "UrlEncoding.h"
#include <memory>
#include <FileModel/Design/FileArchive.h>
#include <FileModel/Design/ComponentHeightTracer.h>
#include <Logger.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include "crow_win.h"
#include "App/RouteController.h"


using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel::Design;
using namespace Utils;

namespace Odb::App::Server
{


	FileModelController::FileModelController(Odb::Lib::App::IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	std::optional<crow::response> FileModelController::TryGetFileArchive(
		const std::string& designName,
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive>& pFileArchive) const
	{
		try
		{
			pFileArchive = m_serverApp.designs().GetFileArchive(designName);
		}
		catch (const std::exception& e)
		{
			// Corrupt / unloadable archive: DesignCache::LoadFileArchive throws so
			// callers can distinguish this from a missing design. Translate to
			// INTERNAL/500. The exception details (which may contain server
			// filesystem paths) are logged server-side only, never sent to the client.
			logexception_msg(e, "failed to load design \"" + designName + "\"");
			return crow::response(crow::status::INTERNAL_SERVER_ERROR,
				"failed to load design \"" + designName + "\": could not extract or parse archive");
		}

		if (pFileArchive == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designName << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		return std::nullopt;
	}

	////FileArchive	*
		////StepDirectory::StringMap m_stepsByName;	//*
			////LayerDirectory::StringMap m_layersByName;	//*
			////	ComponentsFile m_componentsFile;
			////	FeaturesFile m_featuresFile;
			////	AttrListFile m_attrListFile;
			////Netlist//File::StringMap m_netlistsByName;	//*
				
			////EdaDataFile m_edaData;
			////AttrListFile m_attrListFile;
			////FeaturesFile m_profileFile;
			////StepHdrFile m_stepHdrFile;
		
		////SymbolsDirectory::StringMap m_symbolsDirectoriesByName;	//*
			//// AttrList file
			//// features file
	
		////MiscInfoFile m_miscInfoFile;
		////MatrixFile m_matrixFile;
		////StandardFontsFile m_standardFontsFile;	
		////AttrListFile m_miscAttrListFile;

	//	* need endpoints to list of keys of maps, i.e. available names to fetch

	void FileModelController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels")
			([&](const crow::request& req)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->filemodels_list_route_handler(req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>").methods(crow::HTTPMethod::GET)
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->filemodels_get_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>").methods(crow::HTTPMethod::POST)
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->filemodels_post_route_handler(designName, req);
				});			

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_list_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/eda_data")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_edadata_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/attrlist")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_attrlist_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/profile")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_profile_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/stephdr")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_stephdr_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/netlists/<string>")
			([&](const crow::request& req, std::string designName, std::string stepName, std::string netlistName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_netlists_route_handler(designName, stepName, netlistName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/netlists")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_netlists_list_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layers/<string>")
			([&](const crow::request& req, std::string designName, std::string stepName, std::string layerName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_layers_route_handler(designName, stepName, layerName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layers")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_layers_list_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/diagnostics/symbol_units")
			([&](const crow::request& req, std::string designName, std::string stepName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_diagnostics_symbol_units_route_handler(designName, stepName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layers/<string>/components")
			([&](const crow::request& req, std::string designName, std::string stepName, std::string layerName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_layers_components_route_handler(designName, stepName, layerName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layers/<string>/features")
			([&](const crow::request& req, std::string designName, std::string stepName, std::string layerName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_layers_features_route_handler(designName, stepName, layerName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layers/<string>/attrlist")
			([&](const crow::request& req, std::string designName, std::string stepName, std::string layerName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->steps_layers_attrlist_route_handler(designName, stepName, layerName, req);
				});	

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/symbols/<string>")
			([&](const crow::request& req, std::string designName, std::string symbolName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->symbols_route_handler(designName, symbolName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/symbols/<string>/features")
			([&](const crow::request& req, std::string designName, std::string symbolName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->symbols_features_route_handler(designName, symbolName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/symbols/<string>/attrlist")
			([&](const crow::request& req, std::string designName, std::string symbolName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->symbols_attrlist_route_handler(designName, symbolName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/symbols")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->symbols_list_route_handler(designName, req);
				});

		//register_route_handler("/filemodels/misc/attrlist", std::bind(&FileModelController::misc_attrlist_route_handler, this, std::placeholders::_1));
		//register_route_handler("/filemodels/matrix/matrix", std::bind(&FileModelController::matrix_matrix_route_handler, this, std::placeholders::_1));
		//register_route_handler("/filemodels/misc/info", std::bind(&FileModelController::misc_info_route_handler, this, std::placeholders::_1));

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/misc/attrlist")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->misc_attrlist_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/matrix/matrix")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->matrix_matrix_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/misc/info")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->misc_info_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/fonts/standard")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->fonts_standard_route_handler(designName, req);
				});
	}

	crow::response FileModelController::filemodels_get_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}

		return withCacheHeaders(crow::response(JsonCrowReturnable(*pFileArchive)), etag);
	}

	crow::response FileModelController::filemodels_post_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		const auto& json = req.body;
		if (json.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "no data provided in POST body");
		}

		auto fileArchive = std::make_shared<FileArchive>();
		fileArchive->from_json(json);
		m_serverApp.designs().AddFileArchive(designName, fileArchive, false);
		
		return crow::response();
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/eda_data", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		return withCacheHeaders(crow::response(JsonCrowReturnable(edaDataFile)), etag);
	}

	crow::response FileModelController::steps_attrlist_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/attrlist", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		auto& attrListFile = step->GetAttrListFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(attrListFile)), etag);
	}

	crow::response FileModelController::steps_profile_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/profile", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		auto& profileFile = step->GetProfileFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(profileFile)), etag);
	}

	crow::response FileModelController::steps_stephdr_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/stephdr", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		auto& stepHdrFile = step->GetStepHdrFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(stepHdrFile)), etag);
	}

	crow::response FileModelController::steps_netlists_route_handler(const std::string& designName, const std::string& stepName, const std::string& netlistName, const crow::request& req)
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

		auto netlistNameDecoded = UrlEncoding::decode(netlistName);
		if (stepNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "netlist name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/netlists/<string>", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& netlistsByName = step->GetNetlistsByName();
		auto findIt2 = netlistsByName.find(netlistNameDecoded);
		if (findIt2 == netlistsByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " netlist: \"" << netlistNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& netlist = findIt2->second;
		
		return withCacheHeaders(crow::response(JsonCrowReturnable(*netlist)), etag);
	}

	crow::response FileModelController::steps_netlists_list_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& netlistsByName = step->GetNetlistsByName();
		crow::json::wvalue::list netlistNames;
		for (const auto& kvNetlists : netlistsByName)
		{
			netlistNames.push_back(kvNetlists.first);
		}

		crow::json::wvalue jsonResponse;
		jsonResponse["netlists"] = std::move(netlistNames);
		return crow::response(jsonResponse);
	}

	crow::response FileModelController::steps_list_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}

		auto& stepsByName = pFileArchive->GetStepsByName();
		crow::json::wvalue::list stepNames;
		for (const auto& kvSteps : stepsByName)
		{
			stepNames.push_back(kvSteps.first);
		}

		crow::json::wvalue jsonResponse;
		jsonResponse["steps"] = std::move(stepNames);
		return crow::response(jsonResponse);
	}

	crow::response FileModelController::steps_layers_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req)
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

		auto layerNameDecoded = UrlEncoding::decode(layerName);
		if (layerNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "layer name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/layers/<string>", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& layersByName = step->GetLayersByName();
		auto findIt2 = layersByName.find(layerNameDecoded);
		if (findIt2 == layersByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " layer: \"" << layerNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& layer = findIt2->second;

		return withCacheHeaders(crow::response(JsonCrowReturnable(*layer)), etag);
	}

	crow::response FileModelController::steps_layers_components_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req)
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

		auto layerNameDecoded = UrlEncoding::decode(layerName);
		if (layerNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "layer name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/layers/<string>/components", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& layersByName = step->GetLayersByName();
		auto findIt2 = layersByName.find(layerNameDecoded);
		if (findIt2 == layersByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " layer: \"" << layerNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& layer = findIt2->second;

		auto& componentsFile = layer->GetComponentsFile();

		// Trace REST response for target components / failures
		const auto &attributeNames = componentsFile.GetAttributeNames();
		for (const auto &pComponentRecord : componentsFile.GetComponentRecords())
		{
			ComponentHeightTracer::instance().logRestResponse(
				pComponentRecord->compName,
				pComponentRecord->pkgRef,
				pComponentRecord->GetAttributeLookupTable(),
				attributeNames);
		}
		return withCacheHeaders(crow::response(JsonCrowReturnable(componentsFile)), etag);
	}

	crow::response FileModelController::steps_layers_features_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req)
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

		auto layerNameDecoded = UrlEncoding::decode(layerName);
		if (layerNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "layer name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/layers/<string>/features", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& layersByName = step->GetLayersByName();
		auto findIt2 = layersByName.find(layerNameDecoded);
		if (findIt2 == layersByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " layer: \"" << layerNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& layer = findIt2->second;

		auto& featuresFile = layer->GetFeaturesFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(featuresFile)), etag);
	}

	crow::response FileModelController::steps_layers_attrlist_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req)
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

		auto layerNameDecoded = UrlEncoding::decode(layerName);
		if (layerNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "layer name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/layers/<string>/attrlist", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& layersByName = step->GetLayersByName();
		auto findIt2 = layersByName.find(layerNameDecoded);
		if (findIt2 == layersByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " layer: \"" << layerNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& layer = findIt2->second;

		auto& attrlistFile = layer->GetAttrListFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(attrlistFile)), etag);
	}

	crow::response FileModelController::steps_layers_list_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& layersByName = step->GetLayersByName();
		crow::json::wvalue::list layerNames;
		for (const auto& kvLayers : layersByName)
		{
			layerNames.push_back(kvLayers.first);
		}	

		crow::json::wvalue jsonResponse;
		jsonResponse["layers"] = std::move(layerNames);
		return crow::response(jsonResponse);
	}

	crow::response FileModelController::steps_diagnostics_symbol_units_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>/diagnostics/symbol_units", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		const auto& layersByName = step->GetLayersByName();
		crow::json::wvalue::list layers;
		for (const auto& kv : layersByName)
		{
			const auto& layerName = kv.first;
			auto pLayer = kv.second;
			if (!pLayer) continue;

			const auto& featuresFile = pLayer->GetFeaturesFile();
			const auto symbols = Odb::Lib::FileModel::Design::collect_symbols(featuresFile);

			int metricCount = 0;
			int imperialCount = 0;
			int noneCount = 0;
			for (const auto& sym : symbols)
			{
				if (!sym) continue;
				switch (sym->GetUnitType())
				{
				case Odb::Lib::UnitType::Metric: metricCount++; break;
				case Odb::Lib::UnitType::Imperial: imperialCount++; break;
				case Odb::Lib::UnitType::None: noneCount++; break;
				}
			}

			const auto rawUnits = featuresFile.GetUnits();
			const auto expected = Odb::Lib::FileModel::Design::inferred_unit_type_from_features_units(rawUnits);
			const auto hasMixed = (metricCount > 0 && imperialCount > 0);
			const auto hasMismatch =
				(expected == Odb::Lib::UnitType::Metric && imperialCount > 0) ||
				(expected == Odb::Lib::UnitType::Imperial && metricCount > 0);

			crow::json::wvalue layer;
			layer["layer"] = layerName;
			layer["features_units_raw"] = rawUnits;
			layer["symbols_total"] = static_cast<int>(symbols.size());
			layer["symbol_unit_counts"]["metric"] = metricCount;
			layer["symbol_unit_counts"]["imperial"] = imperialCount;
			layer["symbol_unit_counts"]["none"] = noneCount;
			layer["has_mixed_symbol_unit_types"] = hasMixed;
			layer["has_mismatch_vs_features_units"] = hasMismatch;
			layers.push_back(layer);
		}

		crow::json::wvalue result;
		result["design"] = designNameDecoded;
		result["step"] = stepNameDecoded;
		result["layers"] = std::move(layers);
		return withCacheHeaders(crow::response(result), etag);
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/steps/<string>", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		return withCacheHeaders(crow::response(JsonCrowReturnable(*step)), etag);
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/symbols/<string>", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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
		return withCacheHeaders(crow::response(JsonCrowReturnable(*symbol)), etag);
	}

	crow::response FileModelController::symbols_features_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/symbols/<string>/features", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& featuresFile = symbol->GetFeaturesFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(featuresFile)), etag);
	}

	crow::response FileModelController::symbols_attrlist_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req)
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

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/symbols/<string>/attrlist", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
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

		auto& attrlistFile = symbol->GetAttrListFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(attrlistFile)), etag);
	}

	crow::response FileModelController::symbols_list_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}		

		auto& symbolsByName = pFileArchive->GetSymbolsDirectoriesByName();
		crow::json::wvalue::list symbolNames;
		for (const auto& kvSymbols : symbolsByName)
		{
			symbolNames.push_back(kvSymbols.first);
		}

		crow::json::wvalue jsonResponse;
		jsonResponse["symbols"] = std::move(symbolNames);
		return crow::response(jsonResponse);
	}	

	crow::response FileModelController::filemodels_list_route_handler(const crow::request& req)
	{
		return makeLoadedFileModelsResponse(false);
	}

	crow::response FileModelController::misc_attrlist_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/misc/attrlist", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}		

		auto& miscAttrListFile = pFileArchive->GetMiscAttrListFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(miscAttrListFile)), etag);
	}

	crow::response FileModelController::matrix_matrix_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/matrix/matrix", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}

		auto& matrixFile = pFileArchive->GetMatrixFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(matrixFile)), etag);
	}

	crow::response FileModelController::misc_info_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/misc/info", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}

		auto& miscInfoFile = pFileArchive->GetMiscInfoFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(miscInfoFile)), etag);
	}

	crow::response FileModelController::fonts_standard_route_handler(const std::string& designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		std::string etag;
		if (auto notModified = checkConditionalGet(req, designNameDecoded, "/filemodels/<string>/fonts/standard", etag)) return std::move(*notModified);
		std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive> pFileArchive;
		if (auto errorResponse = TryGetFileArchive(designNameDecoded, pFileArchive))
		{
			return std::move(*errorResponse);
		}

		auto& standardFontsFile = pFileArchive->GetStandardFontsFile();
		return withCacheHeaders(crow::response(JsonCrowReturnable(standardFontsFile)), etag);
	}
}