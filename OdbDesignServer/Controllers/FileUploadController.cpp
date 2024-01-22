#include "FileUploadController.h"

using namespace std::filesystem;
using namespace Odb::Lib::App;

namespace Odb::App::Server
{
	FileUploadController::FileUploadController(IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void FileUploadController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/designs/upload/<string>")
            .methods(crow::HTTPMethod::POST)
			([&](const crow::request& req, std::string filename)
				{
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

					const auto& contentType = req.get_header_value("Content-Type");
					if (contentType != "application/octet-stream")
					{
                        return crow::response(crow::status::BAD_REQUEST, "unsupported content type: this endpoint only accepts 'applicaiton/octet-stream'");                        
					}

                    return handleOctetStreamUpload(filename, req);
				});

        CROW_ROUTE(m_serverApp.crow_app(), "/designs/upload")
            .methods(crow::HTTPMethod::POST)
            ([&](const crow::request& req)
                {
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

                    const auto& contentType = req.get_header_value("Content-Type");
                    if (contentType.find("multipart/form-data") != 0)
                    {
                        // "Content-Type" header doesn't start with "multipart/form-data"
                        return crow::response(crow::status::BAD_REQUEST, "unsupported content type: this endpoint only accepts 'multipart/form-data'");                        
                    }
                    
                    return handleMultipartFormUpload(req);
                });

        CROW_ROUTE(m_serverApp.crow_app(), "/designs/list")
            .methods(crow::HTTPMethod::GET)
            ([&](const crow::request& req)
                {
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

                    return makeLoadedDesignsResponse();
                });

//        CROW_ROUTE(m_serverApp.crow_app(), "/designs/list/<string>").methods(crow::HTTPMethod::GET)
//            ([&](const crow::request& req, std::string query)
//                {
//                    if (! m_serverApp.designs().isQueryValid(query))                    
//                    {
//                        return crow::response(crow::status::BAD_REQUEST, "invalid query");
//                    }
//
//                    auto designNames = m_serverApp.designs().getUnloadedNames(query);
//                    if (designNames.empty())
//                    {
//                        return crow::response(crow::status::NOT_FOUND, "no matching design names found");                        
//                    }
//
//                    crow::json::wvalue jsonResponse;
//                    jsonResponse["names"] = designNames;
//
//#if defined(_DEBUG)
//                    auto j = jsonResponse.dump();
//#endif
//
//                    return crow::response(jsonResponse);
//                });
	}
    
    crow::response FileUploadController::handleOctetStreamUpload(const std::string& filename, const crow::request& req)
    {
        const auto tempPath = temp_directory_path() / std::tmpnam(nullptr);
        std::ofstream outfile(tempPath, std::ofstream::binary);
        outfile << req.body;
        outfile.close();

        // TODO: sanitize provided filename
        auto safeName = sanitizeFilename(filename);

        path finalPath(m_serverApp.args().designsDir());
        finalPath /= safeName;
        rename(tempPath, finalPath);

        std::string responseBody = "{ \"filename\": \"" + safeName + "\" }";

        return makeLoadedDesignsResponse();
    }

    crow::response FileUploadController::handleMultipartFormUpload(const crow::request& req)
    {
        crow::multipart::message file_message(req);
        for (const auto& part : file_message.part_map)
        {
            const auto& part_name = part.first;
            const auto& part_value = part.second;

            CROW_LOG_DEBUG << "Part: " << part_name;
            if ("InputFile" != part_name)
            {
                // log to debug and skip rest of the loop
                CROW_LOG_DEBUG << " Value: " << part_value.body << '\n';
                continue;
            }

            // Extract the file name
            auto headers_it = part_value.headers.find("Content-Disposition");
            if (headers_it == part_value.headers.end())
            {
                CROW_LOG_ERROR << "No Content-Disposition found";
                return crow::response(400);
            }
            auto params_it = headers_it->second.params.find("filename");
            if (params_it == headers_it->second.params.end())
            {
                CROW_LOG_ERROR << "Part with name \"InputFile\" should have a file";
                return crow::response(400);
            }
            const std::string outfile_name = params_it->second;

            for (const auto& part_header : part_value.headers)
            {
                const auto& part_header_name = part_header.first;
                const auto& part_header_val = part_header.second;
                CROW_LOG_DEBUG << "Header: " << part_header_name << '=' << part_header_val.value;
                for (const auto& param : part_header_val.params)
                {
                    const auto& param_key = param.first;
                    const auto& param_val = param.second;
                    CROW_LOG_DEBUG << " Param: " << param_key << ',' << param_val;
                }
            }

            // Create a new file with the extracted file name and write file contents to it
            const auto tempPath = temp_directory_path() / std::tmpnam(nullptr);
            std::ofstream out_file(tempPath);
            if (!out_file)
            {
                CROW_LOG_ERROR << " Write to file failed\n";
                continue;
            }
            out_file << part_value.body;
            out_file.close();

            auto safeName = sanitizeFilename(outfile_name);
            path finalPath(m_serverApp.args().designsDir());
            finalPath /= safeName;
            rename(tempPath, finalPath);

            CROW_LOG_INFO << " Contents written to " << outfile_name << '\n';
        }

        return makeLoadedDesignsResponse();
    }

    crow::response FileUploadController::makeLoadedDesignsResponse() const
    {
        auto unloadedDesignNames = m_serverApp.designs().getUnloadedDesignNames();
        auto loadedFileArchiveNames = m_serverApp.designs().getLoadedFileArchiveNames();
        auto loadedDesignNames = m_serverApp.designs().getLoadedDesignNames();

        crow::json::wvalue::list designs;
        for (const auto& designName : unloadedDesignNames)
        {
            auto loaded = false;
            if (std::find(loadedFileArchiveNames.begin(), loadedFileArchiveNames.end(), designName) != loadedFileArchiveNames.end() ||
                std::find(loadedDesignNames.begin(), loadedDesignNames.end(), designName) != loadedDesignNames.end())
            {
                loaded = true;
            }
            crow::json::wvalue design;
            design["name"] = designName;
            design["loaded"] = loaded;
            designs.push_back(design);
        }
        crow::json::wvalue jsonResponse;
        jsonResponse["designs"] = std::move(designs);

#if defined(_DEBUG)
        auto j = jsonResponse.dump();
#endif

        return crow::response(jsonResponse);
    }

    std::string FileUploadController::sanitizeFilename(const std::string& filename) const
    {
        return filename;
    }
}