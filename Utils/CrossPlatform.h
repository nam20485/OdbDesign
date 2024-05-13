#pragma once

//#include <corecrt.h>
#include "utils_export.h"
#include <string>
#include <ctime>
#include <time.h>

namespace Utils
{
	class UTILS_EXPORT CrossPlatform
	{
	public:
		static bool localtime_safe(const std::time_t* time, struct std::tm& tmOut);
		static bool getenv_safe(const char* env_var, std::string& envValueOut);
		static bool tmpnam_safe(std::string& tempNameOut);

	};
}
