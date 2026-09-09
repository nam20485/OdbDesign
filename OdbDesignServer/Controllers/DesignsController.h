#pragma once

#include "App/RouteController.h"

#include <memory>
#include <optional>
#include <string>

namespace Odb::Lib::ProductModel
{
	class Design;
}

namespace Odb::App::Server
{
	class DesignsController : public Odb::Lib::App::RouteController
	{
	public:

		DesignsController(Odb::Lib::App::IOdbServerApp& serverApp);

		// Inherited via RouteController
		void register_routes() override;

		constexpr inline static const char* kszIncludeFileArchiveQueryParamName = "include_filearchive";

	private:

		// Fetches the ProductModel Design for a design name, translating failures
		// into HTTP responses: corrupt/unloadable archive (propagated from
		// DesignCache::LoadFileArchive via GetDesign) -> engaged optional holding
		// INTERNAL/500; design not found -> engaged optional holding NOT_FOUND/404.
		// On success the optional is disengaged and pDesign is populated.
		std::optional<crow::response> TryGetDesign(
			const std::string& designName,
			std::shared_ptr<Odb::Lib::ProductModel::Design>& pDesign) const;

		crow::response designs_list_route_handler(const crow::request& req);
		crow::response design_route_handler(std::string designName, const crow::request& req);

		crow::response designs_components_route_handler(std::string designName, const crow::request& req);
		crow::response designs_component_route_handler(std::string designName, std::string refDes, const crow::request& req);

		crow::response designs_nets_route_handler(std::string designName, const crow::request& req);
		crow::response designs_net_route_handler(std::string designName, std::string netName, const crow::request& req);

		crow::response designs_parts_route_handler(std::string designName, const crow::request& req);
		crow::response designs_part_route_handler(std::string designName, std::string partName, const crow::request& req);

		crow::response designs_packages_route_handler(std::string designName, const crow::request& req);
		crow::response designs_package_route_handler(std::string designName, std::string packageName, const crow::request& req);	

	};
}
