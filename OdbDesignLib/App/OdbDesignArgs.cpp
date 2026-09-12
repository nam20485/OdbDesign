#include "OdbDesignArgs.h"
#include <iostream>
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

	std::string OdbDesignArgs::bindAddress() const
	{
		return stringArg("bind", DEFAULT_BIND_ADDRESS);
	}

	int OdbDesignArgs::grpcPort() const
	{
		return intArg("grpc-port", DEFAULT_GRPC_PORT);
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

	bool OdbDesignArgs::httpTrace() const
	{
		return boolArg("http-trace", DEFAULT_HTTP_TRACE);
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

	int OdbDesignArgs::cacheMaxMb() const
	{
		// intArg -> std::stoi throws when CommandLineArgs stored the literal
		// boolean `true` (missing value, or a value starting with '-' or '/');
		// this runs unconditionally at the top of OdbAppBase::Run(), so a
		// malformed --cache-max-mb must not std::terminate the process — fall
		// back to the default with a warning. Negative values clamp to 0
		// (eviction disabled), matching setCacheMaxBytes semantics.
		try
		{
			const int mb = intArg("cache-max-mb", DEFAULT_CACHE_MAX_MB);
			return mb < 0 ? 0 : mb;
		}
		catch (const std::exception&)
		{
			std::cerr << "WARNING: invalid --cache-max-mb value; using default "
				<< DEFAULT_CACHE_MAX_MB << std::endl;
			return DEFAULT_CACHE_MAX_MB;
		}
	}

	int OdbDesignArgs::maxBackgroundLoads() const
	{
		// Same stoi-hardening as cacheMaxMb(): a malformed --max-background-loads
		// must not std::terminate the process — fall back to the default with a
		// warning. Negative values clamp to 0 (unbounded), matching
		// DesignCache::setMaxBackgroundLoads semantics.
		try
		{
			const int loads = intArg("max-background-loads", DEFAULT_MAX_BACKGROUND_LOADS);
			return loads < 0 ? 0 : loads;
		}
		catch (const std::exception&)
		{
			std::cerr << "WARNING: invalid --max-background-loads value; using default "
				<< DEFAULT_MAX_BACKGROUND_LOADS << std::endl;
			return DEFAULT_MAX_BACKGROUND_LOADS;
		}
	}

	std::string OdbDesignArgs::getUsageString() const
	{
		std::stringstream ss;
		ss << "Usage: " << executableName() << " [options]\n";
		ss << "Options:\n";
		ss << "  --bind <ip|host>        Bind address for REST server (default: " << DEFAULT_BIND_ADDRESS << ")\n";
		ss << "  --port <port>            Port to listen on (default: " << DEFAULT_PORT << ")\n";
		ss << "  --grpc-port <port>       gRPC port to listen on (default: " << DEFAULT_GRPC_PORT << ")\n";
		ss << "  --designs-dir <dir>      Directory containing design files (default: " << DEFAULT_DESIGNS_DIR << ")\n";
		ss << "  --templates-dir <dir>    Directory containing template files (default: " << DEFAULT_TEMPLATES_DIR << ")\n";
		ss << "  --http-trace             Enable per-request HTTP tracing (default: " << (DEFAULT_HTTP_TRACE ? "true" : "false") << ")\n";
		ss << "  --load-design <design>   Design to load on startup (default: " << DEFAULT_LOAD_DESIGN << ")\n";
		ss << "  --load-all               Load all designs on startup (default: " << (DEFAULT_LOAD_ALL ? "true" : "false") << ")\n";
		ss << "  --disable-authentication Disable authentication (default: " << (DEFAULT_DISABLE_AUTH ? "true" : "false") << ")\n";
		ss << "  --cache-max-mb <MB>      Max design cache size in MB before LRU eviction, 0 disables (default: " << DEFAULT_CACHE_MAX_MB << ")\n";
		ss << "  --max-background-loads <N>  Max concurrent background design loads, 0 = unbounded (default: " << DEFAULT_MAX_BACKGROUND_LOADS << ")\n";
		ss << "  --help                   Print this help message\n";
		return ss.str();		
	}	
}
