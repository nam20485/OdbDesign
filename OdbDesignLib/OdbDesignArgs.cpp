#include "OdbDesignArgs.h"
#include "OdbDesignArgs.h"
#include "OdbDesignArgs.h"
#include "OdbDesignArgs.h"

namespace Odb::Lib
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

	std::string Odb::Lib::OdbDesignArgs::getUsageString() const
	{
		return "OdbDesignServer usage:\n"
			"\n"
			"  OdbDesignServer [options]\n"
			"\n"
			"OPTIONS:\n"
			"  --port <port>            Port to listen on (default: 8080)\n"
			"  --use-https              Use HTTPS (default: false)\n"
			"  --ssl-dir <dir>          Directory containing SSL certificate and key files (default: ./ssl)\n"
			"  --designs-dir <dir>      Directory containing design files (default: ./designs)\n"
			"  --templates-dir <dir>    Directory containing template files (default: ./templates)\n"
			"  --help                   Print this help message\n";
	}
}
