#pragma once

#include "RouteController.h"


namespace Odb::App::Server
{
	class StepsEdaDataController : public RouteController
	{
	public:
		StepsEdaDataController(OdbDesignServerApp* pServerApp);

		void AddRoutes() override;
	};
}
