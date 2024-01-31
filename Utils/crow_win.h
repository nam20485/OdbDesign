#pragma once

// disable error-checking in crow's json code to improve its performance
// https://crowcpp.org/master/guides/json/
//#define CROW_JSON_NO_ERROR_CHECK

#include "win.h"
#include "crow.h"
#include "crow/middlewares/cors.h"


using CrowApp = crow::Crow<crow::CORSHandler>;