#include "OdbDesignArgs.h"
#include <sstream>

namespace Odb::Lib::App
{
	OdbDesignArgs::OdbDesignArgs(int argc, char* argv[])
		: CommandLineArgs(argc, argv)
	{
	}

	int OdbDesignArgs::port() const
	{
		return intArg("port", DEFAULT_PORT);
	}

	bool OdbDesignArgs::useHttps() const
	{
		return boolArg("use-https", DEFAULT_USE_HTTPS);
	}

	std::string OdbDesignArgs::sslDir() const
	{
		return stringArg("ssl-dir", DEFAULT_SSL_DIR);
	}

	std::string OdbDesignArgs::designsDir() const
	{
		return stringArg("designs-dir", DEFAULT_DESIGNS_DIR);
	}

	std::string OdbDesignArgs::templatesDir() const
	{
		return stringArg("templates-dir", DEFAULT_TEMPLATES_DIR);
	}

	bool OdbDesignArgs::help() const
	{
		return boolArg("help", DEFAULT_HELP);
	}

	std::string OdbDesignArgs::loadDesign() const
	{
		return stringArg("load-design", DEFAULT_LOAD_DESIGN);
	}

	bool OdbDesignArgs::loadAll() const
	{
		return boolArg("load-all", DEFAULT_LOAD_ALL);
	}

	bool OdbDesignArgs::disableAuthentication() const
	{
		return boolArg("disable-authentication", DEFAULT_DISABLE_AUTH);
	}

	std::string OdbDesignArgs::getUsageString() const
	{
		std::stringstream ss;
		ss << "Usage: " << executableName() << " [options]\n";
		ss << "Options:\n";
		ss << "  --port <port>            Port to listen on (default: " << DEFAULT_PORT << ")\n";
		ss << "  --use-https              Use HTTPS (default: " << (DEFAULT_USE_HTTPS ? "true" : "false") << ")\n";
		ss << "  --ssl-dir <dir>          Directory containing SSL certificate and key files (default: " << DEFAULT_SSL_DIR << ")\n";
		ss << "  --designs-dir <dir>      Directory containing design files (default: " << DEFAULT_DESIGNS_DIR << ")\n";
		ss << "  --templates-dir <dir>    Directory containing template files (default: " << DEFAULT_TEMPLATES_DIR << ")\n";
		ss << "  --load-design <design>   Design to load on startup (default: " << DEFAULT_LOAD_DESIGN << ")\n";
		ss << "  --load-all               Load all designs on startup (default: " << (DEFAULT_LOAD_ALL ? "true" : "false") << ")\n";
		ss << "  --disable-authentication Disable authentication (default: " << (DEFAULT_DISABLE_AUTH ? "true" : "false") << ")\n";
		ss << "  --help                   Print this help message\n";
		return ss.str();		
	}	
}
