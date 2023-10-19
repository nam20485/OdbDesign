#pragma once

#include "RouteController.h"
#include "IOdbServerApp.h"


namespace Odb::App::Server
{
	class HelloWorldController : public Odb::Lib::RouteController
	{
	public:
		HelloWorldController(Odb::Lib::IOdbServerApp& serverApp);

		void register_routes() override;

	};
}
