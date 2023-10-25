#pragma once

#include "CommandLineArgs.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT OdbDesignArgs : public Utils::CommandLineArgs
	{
	public:
		OdbDesignArgs(int argc, char* argv[]);

		int port() const;
		bool useHttps() const;
		std::string sslDir() const;
		std::string designsDir() const;
		std::string templatesDir() const;
		bool help() const;

		const int DEFAULT_PORT = 8888;
		const bool DEFAULT_USE_HTTPS = false;
		const char* DEFAULT_SSL_DIR = "./ssl";
		const char* DEFAULT_DESIGNS_DIR = "designs";
		const char* DEFAULT_TEMPLATES_DIR = "templates";
		const bool DEFAULT_HELP = false;

		// Inherited via CommandLineArgs
		std::string getUsageString() const override;

	};
}
