#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"


namespace Odb::App::Server
{
	class HelloWorldController : public Odb::Lib::RouteController
	{
	public:
		HelloWorldController(Odb::Lib::IOdbServerApp& serverApp);

		void register_routes() override;

	};
}
