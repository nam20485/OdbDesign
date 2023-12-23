#include "CrowHttpServer.h"
#include "crow_win.h"

namespace Odb::Lib::App
{
    void CrowHttpServer::setLogLevel(LogLevel level)
    {
        m_crowApp.loglevel(static_cast<crow::LogLevel>(level));

  //      switch (level)
  //      {
  //          case LogLevel::Debug:
  //              m_crowApp.loglevel(crow::LogLevel::Debug);
  //              break;            
		//	case LogLevel::Info:
		//		m_crowApp.loglevel(crow::LogLevel::Info);
		//		break;
  //          case LogLevel::Warning:
		//		m_crowApp.loglevel(crow::LogLevel::Warning);
		//		break;
		//	case LogLevel::Error:
		//		m_crowApp.loglevel(crow::LogLevel::Error);
		//		break;
		//	case LogLevel::Critical:
		//		m_crowApp.loglevel(crow::LogLevel::Critical);
		//		break;
  //          default:
  //              // unreachable code
  //              m_crowApp.loglevel(crow::LogLevel::Info);
  //              break;
		//}
    }

    void CrowHttpServer::useCompression(CompressionType compressionType)
    {
        switch (compressionType)
        {
            case CompressionType::GZip:
				m_crowApp.use_compression(crow::compression::algorithm::GZIP);
				break;
            case CompressionType::Deflate:
                m_crowApp.use_compression(crow::compression::algorithm::DEFLATE);
                break;
        }
    }

    void CrowHttpServer::setSslKeyFiles(const std::string& certFile, const std::string& keyFile)
    {
        m_crowApp.ssl_file(certFile, keyFile);
    }

    void CrowHttpServer::setPort(unsigned int port)
    {
        m_crowApp.port(static_cast<uint16_t>(port));
    }

    void CrowHttpServer::setMultithreaded(bool multiThreaded)
    {
        if (multiThreaded)
        {
            m_crowApp.multithreaded();
        }
    }

    void CrowHttpServer::run()
    {
        m_crowApp.run();
    }

    void CrowHttpServer::addRoute(const std::string& route, HttpMethod method, TRouteHandlerFunction handler)
    {
        //m_crowApp.route<crow::black_magick::get_parameter_tag(url)>(url);
        //m_crowApp.route_dynamic(route);


        m_crowApp.route_dynamic(const_cast<std::string&&>(route));
        	([&, handler](const crow::request& req, std::string designName, std::string stepName)
        		{
                    HttpRequest request = convertRequest(req);                    

                    //return makeResponse(handler(makeRequest(req)));
                    auto response = handler(request);

                    crow::response crowResponse = convertResponse(response);
                    return crowResponse;

        			//return this->steps_edadata_route_handler(designName, stepName, req);
        		});
    }

    HttpRequest CrowHttpServer::convertRequest(const crow::request& request) const
    {
        HttpRequest httpRequest;
        request.

        return httpRequest;
    }

    crow::response CrowHttpServer::convertResponse(const HttpResponse& response) const
    {
        return crow::response();
    }
}