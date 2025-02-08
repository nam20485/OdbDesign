#include "CrossPlatform.h"
#include "macros.h"
#include <ctime>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <mutex>
#include <cstdio>
#include <ctime>

namespace Utils
{
	std::mutex localtimeMutex;

	bool CrossPlatform::localtime_safe(const std::time_t* time, struct std::tm& tmOut)
	{
		#if (IS_WINDOWS)
		{
			return (0 == localtime_s(&tmOut, time));
		}
		#elif (IS_LINUX || IS_APPLE)
		{	
			std::lock_guard lock(localtimeMutex);
			if (nullptr != localtime_r(time, &tmOut))
			{
				return true;
			}
		}
		#endif
		return false;
	}

	bool CrossPlatform::getenv_safe(const char* env_var, std::string& envValueOut)
	{
		#if (IS_WINDOWS)
		{
			char* envValue = nullptr;
			size_t len = 0;
			if (0 == _dupenv_s(&envValue, &len, env_var))
			{
				if (envValue != nullptr && len > 0)
				{
					envValueOut = envValue;
					free(envValue);
					return true;
				}
			}
		}
		#elif (IS_LINUX || IS_APPLE)
		{
			auto val = std::getenv(env_var);
			if (val != nullptr)
			{
				envValueOut = val;
				return true;
			}

		}
		#endif

		return false;
	}
		
	bool CrossPlatform::tmpnam_safe(std::string& tempNameOut)
	{
		#if (IS_WINDOWS)
		{
			char szTempName[L_tmpnam_s];
			if (0 == tmpnam_s(szTempName, L_tmpnam_s))
			{
				tempNameOut = szTempName;
				return true;
			}
		}
		#elif (IS_LINUX || IS_APPLE)
		{
			// mkstemp
			char szTempName[L_tmpnam]{ 0 };
			if (nullptr != std::tmpnam(szTempName))
			{
				tempNameOut = szTempName;
				return true;
			}
		}
		#endif				
		
		return false;
	}
}