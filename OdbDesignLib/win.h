#pragma once

#define WIN32_LEAN_AND_MEAN

#include <WinSDKVer.h>

#ifndef _WIN32_WINNT_WIN7
#	define _WIN32_WINNT_WIN7	0x0601
#endif

#ifdef _MSC_VER
#	define _WIN32_WINNT			_WIN32_WINNT_WIN7
#	define WINVER				_WIN32_WINNT
#endif

#include <SDKDDKVer.h>
//#include <windows.h>