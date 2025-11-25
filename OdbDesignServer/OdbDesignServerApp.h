#pragma once

#include "OdbServerAppBase.h"
#include <memory>
#include <string>
#include <App/DesignCache.h>


namespace Odb::App::Server
{
	class OdbDesignServerApp : public Odb::Lib::App::OdbServerAppBase
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		//~OdbDesignServerApp();	
				
		//Utils::ExitCode Run() override;		

	protected:												
		void add_controllers() override;

		// Override to implement gRPC service
		void RunGrpcServer(const std::string& server_address, std::shared_ptr<Odb::Lib::App::DesignCache> cache) override;

		// Inherited via OdbServerAppBase
		bool preServerRun() override;
		//bool postServerRun() override;

	};
}
