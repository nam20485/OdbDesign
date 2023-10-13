#pragma once

#include "OdbDesignServer.h"
#include "OdbServerAppBase.h"

using namespace Odb::Lib;

namespace Odb::App::Server
{
	class OdbDesignServerApp : public OdbServerAppBase
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		~OdbDesignServerApp();	
				
		ExitCode Run() override;		

	protected:												
		void add_controllers() override;

	};
}
