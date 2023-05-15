#pragma once

inline bool IsMsvc()
{
#if defined(_MSC_VER)
	return true;
#else
	return false;
#endif
}