#include "CrossPlatform.h"
#include "macros.h"


#include <ctime>
#include <cstdlib>

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
			localtime_r(time, &tmOut);
			return true;
		}
		#endif		
	}

	//char* CrossPlatform::getenv_safe(const char* env_var)
	//{
	//	#if (IS_WINDOWS)
	//	{
	//		//_dupenv_s()			
	//	}
	//	#elif (IS_LINUX || IS_APPLE)
	//	{
	//		localtime_r(time, &tmOut);
	//		return true;
	//	}
	//	#endif	
	//}

	//char* CrossPlatform::tmpnam(char* filename)
	//{
	//	return nullptr;
	//}
}