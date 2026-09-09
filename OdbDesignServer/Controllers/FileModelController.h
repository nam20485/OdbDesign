#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"

#include <memory>
#include <optional>
#include <string>

namespace Odb::Lib::FileModel::Design
{
	class FileArchive;
}

namespace Odb::App::Server
{
	class FileModelController : public Odb::Lib::App::RouteController
	{
	public:
		FileModelController(Odb::Lib::App::IOdbServerApp& serverApp);
		//virtual ~FileModelController() = default;

		void register_routes() override;

	private:
		// Fetches the FileArchive for a design, translating failures into HTTP
		// responses: corrupt/unloadable archive (DesignCache::LoadFileArchive
		// throws) -> engaged optional holding INTERNAL/500; design not found ->
		// engaged optional holding NOT_FOUND/404. On success the optional is
		// disengaged and pFileArchive is populated.
		std::optional<crow::response> TryGetFileArchive(
			const std::string& designName,
			std::shared_ptr<Odb::Lib::FileModel::Design::FileArchive>& pFileArchive) const;
		crow::response filemodels_get_route_handler(const std::string& designName, const crow::request& req);
		crow::response filemodels_post_route_handler(const std::string& designName, const crow::request& req);
		crow::response filemodels_list_route_handler(const crow::request& req);

		crow::response steps_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_edadata_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);		
		crow::response steps_attrlist_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_profile_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_stephdr_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_netlists_route_handler(const std::string& designName, const std::string& stepName, const std::string& netlistName, const crow::request& req);
		crow::response steps_netlists_list_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_list_route_handler(const std::string& designName, const crow::request& req);

		crow::response steps_layers_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req);
		crow::response steps_layers_components_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req);
		crow::response steps_layers_features_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req);
		crow::response steps_layers_attrlist_route_handler(const std::string& designName, const std::string& stepName, const std::string& layerName, const crow::request& req);
		crow::response steps_layers_list_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);
		crow::response steps_diagnostics_symbol_units_route_handler(const std::string& designName, const std::string& stepName, const crow::request& req);

		crow::response symbols_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req);
		crow::response symbols_features_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req);
		crow::response symbols_attrlist_route_handler(const std::string& designName, const std::string& symbolName, const crow::request& req);
		crow::response symbols_list_route_handler(const std::string& designName, const crow::request& req);

		crow::response misc_attrlist_route_handler(const std::string& designName, const crow::request& req);
		crow::response matrix_matrix_route_handler(const std::string& designName, const crow::request& req);
		crow::response misc_info_route_handler(const std::string& designName, const crow::request& req);
		crow::response fonts_standard_route_handler(const std::string& designName, const crow::request& req);

	};
}
