#pragma once

#include "OdbDesignServer.h"
#include "App/OdbServerAppBase.h"


namespace Odb::App::Server
{
	class OdbDesignServerApp : public Odb::Lib::App::OdbServerAppBase
	{
	public:
		OdbDesignServerApp(int argc, char* argv[]);
		//~OdbDesignServerApp();	
				
		//Utils::ExitCode Run() override;		

		static OdbDesignServerApp* inst_;
		// store last heartbeat time
		std::atomic<std::chrono::steady_clock::time_point> lastHeartbeat_;
		void updateLastHeartbeat()
		{
			lastHeartbeat_.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
		}

	protected:
		void add_controllers() override;


		// Inherited via OdbServerAppBase
		bool preServerRun() override;
		//bool postServerRun() override;

	};
}
