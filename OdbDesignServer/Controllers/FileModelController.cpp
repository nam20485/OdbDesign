#include "FileModelController.h"
#include <JsonCrowReturnable.h>
#include "UrlEncoding.h"
#include <memory>
#include <FileModel/Design/FileArchive.h>
#include <sstream>
#include <string>
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

		CROW_ROUTE(m_serverApp.crow_app(), "/filemodels/<string>/steps/<string>/layer/<string>/components")
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

		auto pFileArchive = m_serverApp.designs().GetFileArchive(designNameDecoded);
		if (pFileArchive == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		return crow::response(JsonCrowReturnable(*pFileArchive));
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
		auto& attrListFile = step->GetAttrListFile();
		return crow::response(JsonCrowReturnable(attrListFile));
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
		auto& profileFile = step->GetProfileFile();
		return crow::response(JsonCrowReturnable(profileFile));
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
		auto& stepHdrFile = step->GetStepHdrFile();
		return crow::response(JsonCrowReturnable(stepHdrFile));
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

		auto& netlistsByName = step->GetNetlistsByName();
		auto findIt2 = netlistsByName.find(netlistNameDecoded);
		if (findIt2 == netlistsByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " netlist: \"" << netlistNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& netlist = findIt2->second;
		
		return crow::response(JsonCrowReturnable(*netlist));
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

		auto pFileArchive = m_serverApp.designs().GetFileArchive(designNameDecoded);
		if (pFileArchive == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
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

		auto& layersByName = step->GetLayersByName();
		auto findIt2 = layersByName.find(layerNameDecoded);
		if (findIt2 == layersByName.end())
		{
			std::stringstream ss;
			ss << "(design: \"" << designNameDecoded << "\")" << " step: \"" << stepNameDecoded << "\"" << " layer: \"" << layerNameDecoded << "\"" << " not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}
		auto& layer = findIt2->second;

		return crow::response(JsonCrowReturnable(*layer));
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
		return crow::response(JsonCrowReturnable(componentsFile));
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
		return crow::response(JsonCrowReturnable(featuresFile));
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
		return crow::response(JsonCrowReturnable(attrlistFile));
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

		auto& featuresFile = symbol->GetFeaturesFile();
		return crow::response(JsonCrowReturnable(featuresFile));
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

		auto& attrlistFile = symbol->GetAttrListFile();
		return crow::response(JsonCrowReturnable(attrlistFile));
	}

	crow::response FileModelController::symbols_list_route_handler(const std::string& designName, const crow::request& req)
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
		return makeLoadedFileModelsResponse();
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