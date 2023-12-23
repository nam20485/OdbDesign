#pragma once

#include "../odbdesign_export.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IHttpServer
	{
	public:
		enum class LogLevel
		{
			Debug = 0,
			Info,
			Warning,
			Error,
			Critical
		};

		enum class CompressionType
		{
			GZip,
			Deflate
		};

		enum class HttpMethod
		{
			Delete = 0,
			Get,
			Head,
			Post,
			Put,

			Connect,
			Options,
			Trace,

			Patch,
			Purge,
		};

		typedef std::function<HttpResponse(const HttpRequest& request)> TRouteHandlerFunction;		

		virtual void setLogLevel(LogLevel level) = 0;
		virtual void useCompression(CompressionType compressionType) = 0;
		virtual void setSslKeyFiles(const std::string& certFile, const std::string& keyFile) = 0;
		virtual void setPort(unsigned int port) = 0;
		virtual void setMultithreaded(bool multiThreaded) = 0;
		
		virtual void run() = 0;	

		virtual void addRoute(const std::string& route, HttpMethod method, TRouteHandlerFunction handler) = 0;

	};
}
