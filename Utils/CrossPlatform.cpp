#include "CrossPlatform.h"
#include "macros.h"
#include <ctime>
#include <cstdlib>
#include <malloc.h>
#include <string>
#include <time.h>

namespace Utils
{
	bool CrossPlatform::localtime_safe(const std::time_t* time, struct std::tm& tmOut)
	{
		#if (IS_WINDOWS)
		{
			return (0 == localtime_s(&tmOut, time));
		}
		#elif (IS_LINUX || IS_APPLE)
		{	
			if (nullptr != localtime_r(time, &tmOut))
			{
				return true;
			}
		}
		#endif
		return false;
	}

	bool CrossPlatform::getenv(const char* env_var, std::string& envValueOut)
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
			//mkstemp
		#endif				
		
		return false;
	}
}