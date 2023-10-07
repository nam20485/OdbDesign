#pragma once

#include "../OdbDesignServer.h"
#include "crow.h"


class RouteController
{
public:
	RouteController(crow::SimpleApp& crowApp);

	virtual void AddRoutes() = 0;
	
protected:
	crow::SimpleApp& m_crowApp;

};
