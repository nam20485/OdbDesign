#pragma once

#ifdef _MSC_VER

	#define WIN32_LEAN_AND_MEAN

	#include <WinSDKVer.h>

	#ifndef _WIN32_WINNT_WIN7
	#	define _WIN32_WINNT_WIN7	0x0601
	#endif

	#define _WIN32_WINNT	_WIN32_WINNT_WIN7
	#define WINVER			_WIN32_WINNT

	#include <SDKDDKVer.h>
	//#include <windows.h>

#endif