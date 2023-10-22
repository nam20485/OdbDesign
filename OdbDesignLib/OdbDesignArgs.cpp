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

	std::string OdbDesignArgs::designsDir() const
	{
		return stringArg("designs-dir", DEFAULT_DESIGNS_DIR);
	}

	std::string OdbDesignArgs::templatesDir() const
	{
		return stringArg("templates-dir", DEFAULT_TEMPLATES_DIR);
	}
}
