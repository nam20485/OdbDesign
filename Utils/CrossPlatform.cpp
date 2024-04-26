#include "CrossPlatform.h"
#include "macros.h"


#include <ctime>
#include <cstdlib>

namespace Utils
{
	//bool CrossPlatform::localtime(const std::time_t* time, struct std::tm& tmOut)
	//{
	//	#if (Utils::IsWindows())
	//	{
	//		if (0 == localtime_s(&tmOut, time))
	//		{
	//			return true;
	//		}
	//		return false;
	//	}
	//	#elseif (Utils::IsLinux() || Utils::IsApple())		
	//	{			
	//		localtime_r(time, &tmOut);
	//		return true;
	//	}
	//	#endif

	//	return false;
	//}

	//char* CrossPlatform::getenv(const char* env_var)
	//{
	//	return nullptr;
	//}

	//char* CrossPlatform::tmpnam(char* filename)
	//{
	//	return nullptr;
	//}
}