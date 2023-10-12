#pragma once

#include "RouteController.h"


namespace Odb::App::Server
{
	// forward declaration
	class OdbDesignServerApp;

	class HelloWorldController : public RouteController
	{
	public:
		HelloWorldController(OdbDesignServerApp* pServerApp);

		void register_routes() override;

	};
}
