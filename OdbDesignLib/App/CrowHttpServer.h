#pragma once

#include "crow_win.h"
#include "IHttpServer.h"
#include "../odbdesign_export.h"


namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT CrowHttpServer : public IHttpServer
	{
	public:

		// Inherited via IHttpServer
		void setLogLevel(LogLevel level) override;
		void useCompression(CompressionType compressionType) override;
		void setSslKeyFiles(const std::string& certFile, const std::string& keyFile) override;
		void setPort(unsigned int port) override;
		void setMultithreaded(bool multiThreaded) override;

		void run() override;

	private:
		crow::SimpleApp m_crowApp;				


		// Inherited via IHttpServer
		void addRoute(const std::string& route, HttpMethod method, TRouteHandlerFunction handler) override;

		HttpRequest convertRequest(const crow::request& request) const;
		crow::response convertResponse(const HttpResponse& response) const;

	};
}
