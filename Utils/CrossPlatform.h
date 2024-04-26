#pragma once

#include <corecrt.h>
#include "utils_export.h"

namespace Utils
{
	class CrossPlatform
	{
	public:
		static bool localtime_safe(const std::time_t* time, struct std::tm& tmOut);
		//static char* getenv(const char* env_var);
		//static char* tmpnam(char* filename);

	};
}
