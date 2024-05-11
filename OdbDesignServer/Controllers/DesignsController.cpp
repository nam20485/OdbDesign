#include "DesignsController.h"
#include <JsonCrowReturnable.h>
#include "UrlEncoding.h"
#include "App/IOdbServerApp.h"
#include "App/RouteController.h"
#include <cstring>
#include <vector>


using namespace Odb::Lib::App;
using namespace Utils;

namespace Odb::App::Server
{
	DesignsController::DesignsController(IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void DesignsController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/designs")
			([&](const crow::request& req)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->designs_list_route_handler(req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->design_route_handler(designName, req);
				});

		CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/components")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->designs_components_route_handler(designName, req);
				});

		//CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/components/<string>")
		//	([&](const crow::request& req, std::string designName, std::string refDes)
		//		{
		//			// authenticate request before sending to handler
		//			auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
		//			if (authResp.code != crow::status::OK)
		//			{
		//				return authResp;
		//			}

		//			return this->designs_component_route_handler(designName, refDes, req);
		//		});

		CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/nets")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->designs_nets_route_handler(designName, req);
				});

		//CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/nets/<string>")
		//	([&](const crow::request& req, std::string designName, std::string netName)
		//		{
		//			// authenticate request before sending to handler
		//			auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
		//			if (authResp.code != crow::status::OK)
		//			{
		//				return authResp;
		//			}

		//			return this->designs_net_route_handler(designName, netName, req);
		//		});

		CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/packages")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->designs_packages_route_handler(designName, req);
				});

		//CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/packages/<string>")
		//	([&](const crow::request& req, std::string designName, std::string packageName)
		//		{
		//			// authenticate request before sending to handler
		//			auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
		//			if (authResp.code != crow::status::OK)
		//			{
		//				return authResp;
		//			}

		//			return this->designs_package_route_handler(designName, packageName, req);
		//		});

		CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/parts")
			([&](const crow::request& req, std::string designName)
				{
					// authenticate request before sending to handler
					auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
					if (authResp.code != crow::status::OK)
					{
						return authResp;
					}

					return this->designs_parts_route_handler(designName, req);
				});

		//CROW_ROUTE(m_serverApp.crow_app(), "/designs/<string>/parts/<string>")
		//	([&](const crow::request& req, std::string designName, std::string partName)
		//		{
		//			// authenticate request before sending to handler
		//			auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
		//			if (authResp.code != crow::status::OK)
		//			{
		//				return authResp;
		//			}

		//			return this->designs_part_route_handler(designName, partName, req);
		//		});		
	}

	crow::response DesignsController::designs_list_route_handler(const crow::request& req)
	{
		return makeLoadedFileModelsResponse();
	}

	crow::response DesignsController::design_route_handler(std::string designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}		

		auto pDesign = m_serverApp.designs().GetDesign(designNameDecoded);
		if (pDesign == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		// TODO: use excludeFileArchive
		bool includeFileArchive = false;
		auto szIncludeFileArchive = req.url_params.get(kszIncludeFileArchiveQueryParamName);
		if (szIncludeFileArchive != nullptr)
		{
			if (std::strcmp(szIncludeFileArchive, "true") == 0 ||
				std::strcmp(szIncludeFileArchive, "yes") == 0)
			{
				includeFileArchive = true;
			}
		}

		if (!includeFileArchive)
		{
			pDesign->ClipFileModel();
		}

		return crow::response(JsonCrowReturnable(*pDesign));
	}
	crow::response DesignsController::designs_components_route_handler(std::string designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto pDesign = m_serverApp.designs().GetDesign(designNameDecoded);
		if (pDesign == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		std::vector<crow::json::rvalue> rvComponents;
		
		const auto& components = pDesign->GetComponents();
		for (const auto& component : components)
		{
			rvComponents.push_back(crow::json::load(component->to_json()));
		}

		crow::json::wvalue wv;
		wv = std::move(rvComponents);
		return crow::response(wv);
	}

	crow::response DesignsController::designs_component_route_handler(std::string designName, std::string refDes, const crow::request& req)
	{
		return crow::response();
	}

	crow::response DesignsController::designs_nets_route_handler(std::string designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto pDesign = m_serverApp.designs().GetDesign(designNameDecoded);
		if (pDesign == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		std::vector<crow::json::rvalue> rvNets;

		const auto& nets = pDesign->GetNets();
		for (const auto& net : nets)
		{
			rvNets.push_back(crow::json::load(net->to_json()));
		}

		crow::json::wvalue wv;
		wv = std::move(rvNets);
		return crow::response(wv);
	}

	crow::response DesignsController::designs_net_route_handler(std::string designName, std::string netName, const crow::request& req)
	{
		return crow::response();
	}

	crow::response DesignsController::designs_parts_route_handler(std::string designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto pDesign = m_serverApp.designs().GetDesign(designNameDecoded);
		if (pDesign == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		std::vector<crow::json::rvalue> rvParts;

		const auto& parts = pDesign->GetParts();
		for (const auto& part : parts)
		{
			rvParts.push_back(crow::json::load(part->to_json()));
		}

		crow::json::wvalue wv;
		wv = std::move(rvParts);
		return crow::response(wv);
	}

	crow::response DesignsController::designs_part_route_handler(std::string designName, std::string partName, const crow::request& req)
	{
		return crow::response();
	}

	crow::response DesignsController::designs_packages_route_handler(std::string designName, const crow::request& req)
	{
		auto designNameDecoded = UrlEncoding::decode(designName);
		if (designNameDecoded.empty())
		{
			return crow::response(crow::status::BAD_REQUEST, "design name not specified");
		}

		auto pDesign = m_serverApp.designs().GetDesign(designNameDecoded);
		if (pDesign == nullptr)
		{
			std::stringstream ss;
			ss << "design: \"" << designNameDecoded << "\" not found";
			return crow::response(crow::status::NOT_FOUND, ss.str());
		}

		std::vector<crow::json::rvalue> rvPackages;

		const auto& packages = pDesign->GetPackages();
		for (const auto& package : packages)
		{
			rvPackages.push_back(crow::json::load(package->to_json()));
		}

		crow::json::wvalue wv;
		wv = std::move(rvPackages);
		return crow::response(wv);
	}

	crow::response DesignsController::designs_package_route_handler(std::string designName, std::string packageName, const crow::request& req)
	{
		return crow::response();
	}
}