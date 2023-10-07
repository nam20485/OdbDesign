#pragma once

#include "Controllers/RouteController.h"

class StepsEdaDataController : public RouteController
{
public:
	StepsEdaDataController(crow::SimpleApp& crowApp);	

	void AddRoutes() override;
};
