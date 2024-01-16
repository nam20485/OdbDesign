#pragma once

#include "win.h"
#include "crow.h"
#include "crow/middlewares/cors.h"


using CrowApp = crow::Crow<crow::CORSHandler>;