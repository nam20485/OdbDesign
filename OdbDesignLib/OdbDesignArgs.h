#pragma once

#include "CommandLineArgs.h"
#include "odbdesign_export.h"

using namespace Utils;

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT OdbDesignArgs : public CommandLineArgs
	{
	public:
		OdbDesignArgs(int argc, char* argv[]);

		int port() const;
		bool useHttps() const;
		std::string designsDir() const;
		std::string templatesDir() const;		

		// port
		// use HTTPS
		// designs dir
		// templates dir
	};
}
