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
		std::string loadDesign() const;
		bool loadAll() const;
		bool disableAuthentication() const;

	protected:
		// Inherited via CommandLineArgs
		std::string getUsageString() const override;

	private:
		constexpr static const int		DEFAULT_PORT =			8888;
		constexpr static const bool		DEFAULT_USE_HTTPS =		false;
		constexpr static const char*	DEFAULT_SSL_DIR =		"./ssl";
		constexpr static const char*	DEFAULT_DESIGNS_DIR =	"designs";
		constexpr static const char*	DEFAULT_TEMPLATES_DIR = "templates";
		constexpr static const bool		DEFAULT_HELP =			false;
		constexpr static const char*	DEFAULT_LOAD_DESIGN =	"";		
		constexpr static const bool		DEFAULT_LOAD_ALL =		false;
		constexpr static const bool		DEFAULT_DISABLE_AUTH = false;

	};
}
