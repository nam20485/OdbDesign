#pragma once


namespace Odb::Lib
{
	static inline bool IsMsvc()
	{
		#if defined(_MSC_VER)
			return true;
		#else
			return false;
		#endif
	}
}