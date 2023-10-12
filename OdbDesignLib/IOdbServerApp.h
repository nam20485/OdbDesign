#pragma once

#include "CommandLineArgs.h"
#include "DesignCache.h"
#include "ExitCode.h"
#include "crow_win.h"

namespace Odb::App::Server
{
	class IOdbServerApp
	{
	public:
		virtual ~IOdbServerApp() {}

		virtual const Utils::CommandLineArgs& arguments() const = 0;
		virtual crow::SimpleApp& crow_app() = 0;
		virtual Odb::Lib::DesignCache& design_cache() = 0;

		virtual Utils::ExitCode Run() = 0;

	protected:
		// abstract class/interface
		IOdbServerApp() = default;
		
	};
}
