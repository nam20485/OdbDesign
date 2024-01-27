#pragma once

#include "App/RouteController.h"

namespace Odb::App::Server
{
	class DesignsController : public Odb::Lib::App::RouteController
	{
	public:

		DesignsController(Odb::Lib::App::IOdbServerApp& serverApp);

		// Inherited via RouteController
		void register_routes() override;

	private:

	};
}
