#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"

namespace Odb::App::Server
{
	class FileUploadController : public Odb::Lib::App::RouteController
	{
	public:
		FileUploadController(Odb::Lib::App::IOdbServerApp& serverApp);

		void register_routes() override;		

	private:
		crow::response handleOctetStreamUpload(const std::string& filename, const crow::request& req);
		crow::response handleMultipartFormUpload(const crow::request& req);

		// TODO: actually implement sanitizeFilename()
		std::string sanitizeFilename(const std::string& filename) const;

		constexpr static const inline char MULTIPART_FORM_DATA_PART_NAME[] = "file";
		constexpr static const inline char MULTIPART_FORM_DATA_PART_FILENAME[] = "filename";
		constexpr static const inline char CONTENT_DISPOSITION_HEADER_NAME[] = "Content-Disposition";
		constexpr static const inline char CONTENT_TYPE_HEADER_NAME[] = "Content-Type";
		constexpr static const inline char CONTENT_TYPE_MULTIPART_FORM_DATA[] = "multipart/form-data";
		constexpr static const inline char CONTENT_TYPE_APPLICATION_OCTET_STREAM[] = "application/octet-stream";
		
		
		//constexpr static const inline char MULTIPART_FORMDATA_PART_NAME[] = "InputFile"
	
	};
}

