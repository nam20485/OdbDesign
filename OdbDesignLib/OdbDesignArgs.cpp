#include "OdbDesignArgs.h"

namespace Odb::Lib
{
	OdbDesignArgs::OdbDesignArgs(int argc, char* argv[])
		: CommandLineArgs(argc, argv)
	{
	}

	int OdbDesignArgs::port() const
	{
		return boolArg("port");
	}

	bool OdbDesignArgs::useHttps() const
	{
		return boolArg("use-https");
	}

	std::string OdbDesignArgs::designsDir() const
	{
		return stringArg("designs-dir");
	}

	std::string OdbDesignArgs::templatesDir() const
	{
		return stringArg("templates-dir");
	}
}
