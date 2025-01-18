#include "FileUploadController.h"
#include "fastmove.h"
#include <system_error>
#include <fstream>
#include <App/RouteController.h>
#include <crow/json.h>

using namespace std::filesystem;
using namespace Odb::Lib::App;
using namespace Utils;

namespace Odb::App::Server
{
	FileUploadController::FileUploadController(IOdbServerApp& serverApp)
		: RouteController(serverApp)
	{
	}

	void FileUploadController::register_routes()
	{
		CROW_ROUTE(m_serverApp.crow_app(), "/files/upload/<string>")
            .methods(crow::HTTPMethod::POST)
			([&](const crow::request& req, std::string filename)
				{
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

					const auto& contentType = req.get_header_value(CONTENT_TYPE_HEADER_NAME);
					if (contentType != CONTENT_TYPE_APPLICATION_OCTET_STREAM)
					{
                        return crow::response(crow::status::BAD_REQUEST, std::string("unsupported content type: this endpoint only accepts '") + CONTENT_TYPE_APPLICATION_OCTET_STREAM + "'");
					}

                    return handleOctetStreamUpload(filename, req);
				});

        CROW_ROUTE(m_serverApp.crow_app(), "/files/upload")
            .methods(crow::HTTPMethod::POST)
            ([&](const crow::request& req)
                {
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

                    const auto& contentType = req.get_header_value(CONTENT_TYPE_HEADER_NAME);
                    if (contentType.find(CONTENT_TYPE_MULTIPART_FORM_DATA) != 0)
                    {
                        // "Content-Type" header doesn't start with "multipart/form-data"
                        return crow::response(crow::status::BAD_REQUEST, std::string("unsupported content type: this endpoint only accepts '") + CONTENT_TYPE_MULTIPART_FORM_DATA + "'");
                    }
                    
                    return handleMultipartFormUpload(req);
                });

        CROW_ROUTE(m_serverApp.crow_app(), "/files/list")
            .methods(crow::HTTPMethod::GET)
            ([&](const crow::request& req)
                {
                    // authenticate request before sending to handler
                    auto authResp = m_serverApp.request_auth().AuthenticateRequest(req);
                    if (authResp.code != crow::status::OK)
                    {
                        return authResp;
                    }

                    return makeLoadedFileModelsResponse();
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

        auto safeName = sanitizeFilename(filename);
        path finalPath(m_serverApp.args().designsDir());
        finalPath /= safeName;

        std::error_code ec;
        fastmove_file(tempPath, finalPath, false, ec);
        if (ec.value() != 0)
        {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, "failed handling new file");
        }

        std::string responseBody = "{ \"filename\": \"" + safeName + "\" }";

        return makeLoadedFileModelsResponse();
    }

    crow::response FileUploadController::handleMultipartFormUpload(const crow::request& req)
    {
        crow::multipart::message file_message(req);
        for (const auto& part : file_message.part_map)
        {
            const auto& part_name = part.first;
            const auto& part_value = part.second;

            CROW_LOG_DEBUG << "Part: " << part_name;
            if (MULTIPART_FORM_DATA_PART_NAME != part_name)
            {
                // log to debug and skip rest of the loop
                CROW_LOG_DEBUG << " Value: " << part_value.body << '\n';
                CROW_LOG_ERROR 
                    << "multipart/form-data POST failed! Part name was: [" << part_name 
                    << "], which is not supported. Part name should be [" << MULTIPART_FORM_DATA_PART_NAME 
                    << "].";
                continue;
            }

            // Extract the file name
            auto headers_it = part_value.headers.find(CONTENT_DISPOSITION_HEADER_NAME);
            if (headers_it == part_value.headers.end())
            {
                CROW_LOG_ERROR << "No " << CONTENT_DISPOSITION_HEADER_NAME << " found";                
                return crow::response(crow::status::BAD_REQUEST);
            }
            auto params_it = headers_it->second.params.find(MULTIPART_FORM_DATA_PART_FILENAME);
            if (params_it == headers_it->second.params.end())
            {
                CROW_LOG_ERROR << "Part with name \"" << "MULTIPART_FORMDATA_PART_NAME" << "\" should have a \"" << "MULTIPART_FORMDATA_PART_FILENAME" << "\" parameter";
                return crow::response(crow::status::BAD_REQUEST);                               
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
            std::ofstream out_file(tempPath, std::ofstream::binary);
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

            std::error_code ec;
            fastmove_file(tempPath, finalPath, false, ec);
            if (ec.value() != 0)
            {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR, "failed handling new file");
            }

            CROW_LOG_INFO << " Contents written to " << outfile_name << '\n';
        }

        return makeLoadedFileModelsResponse();
    }

    std::string FileUploadController::sanitizeFilename(const std::string& filename) const
    {
        // TODO: implement sanitize filename
        return filename;
    }
}