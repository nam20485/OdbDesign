#pragma once

#include "RouteController.h"
#include "IOdbServerApp.h"

namespace Odb::App::Server
{
	class FileUploadController : public Odb::Lib::RouteController
	{
	public:
		FileUploadController(Odb::Lib::IOdbServerApp& serverApp);

		void register_routes() override;		

	private:
		crow::response handleOctetStreamUpload(const std::string& filename, const crow::request& req);
		crow::response handleMultipartFormUpload(const crow::request& req);

		crow::response makeLoadedDesignsResponse() const;

		std::string sanitizeFilename(const std::string& filename) const;
	
	};
}
