#pragma once

#include "RouteController.h"


namespace Odb::App::Server
{
	class HelloWorldController : public RouteController
	{
	public:
		HelloWorldController(OdbDesignServerApp* pServerApp);

		void AddRoutes() override;

	};
}
