#pragma once

#include "CommandLineArgs.h"
#include "odbdesign_export.h"

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT OdbDesignArgs : public Utils::CommandLineArgs
	{
	public:
		OdbDesignArgs(int argc, char* argv[]);

		int port() const;
		bool useHttps() const;
		std::string designsDir() const;
		std::string templatesDir() const;		

		const int DEFAULT_PORT = 8888;
		const bool DEFAULT_USE_HTTPS = false;
		const std::string DEFAULT_DESIGNS_DIR = "designs";
		const std::string DEFAULT_TEMPLATES_DIR = "templates";

	};
}
