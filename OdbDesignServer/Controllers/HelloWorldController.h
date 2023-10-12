#pragma once

#include "RouteController.h"
#include "IOdbServerApp.h"

using namespace Odb::Lib;

namespace Odb::App::Server
{
	class HelloWorldController : public RouteController
	{
	public:
		HelloWorldController(IOdbServerApp* pServerApp);

		void register_routes() override;

	};
}
