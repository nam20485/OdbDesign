#pragma once

#include "RouteController.h"


namespace Odb::App::Server
{
	class StepsEdaDataController : public RouteController
	{
	public:
		StepsEdaDataController(crow::SimpleApp& crowApp);

		void AddRoutes() override;
	};
}
