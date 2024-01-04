#pragma once


namespace Odb::Lib
{
	constexpr static inline bool IsMsvc()
	{
		#if defined(_MSC_VER)
				return true;
		#else
				return false;
		#endif
	}

	constexpr static inline bool IsDebug()
	{
		#if defined(_DEBUG)
				return true;
		#else
				return false;
		#endif
	}

#ifndef ARRAY_COUNT
#	define ARRAY_COUNT(array_) (sizeof(array_)/sizeof((array_)[0]))
#endif

#ifndef CSTR_COUNT
#	define CSTR_COUNT(cstr_) (ARRAY_COUNT(cstr_)-1)
#endif

}