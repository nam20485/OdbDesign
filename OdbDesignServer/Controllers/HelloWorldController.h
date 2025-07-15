#pragma once

#include "App/RouteController.h"
#include "App/IOdbServerApp.h"


namespace Odb::App::Server
{
	class HelloWorldController : public Odb::Lib::App::RouteController
	{
	public:
		HelloWorldController(Odb::Lib::App::IOdbServerApp& serverApp);

		void register_routes() override;

	};
}
