#pragma once

#include "RouteController.h"


class HelloWorldController : public RouteController
{
public:
	HelloWorldController(crow::SimpleApp& crowApp);
	HelloWorldController(crow::SimpleApp& crowApp, const std::string& prefix);

	void AddRoutes() override;

};
