#pragma once

#include <cstdlib>
#include "str_utils.h"


namespace Utils
{

#if defined(_WIN32)		// or _MSC_VER
#	define IS_WINDOWS (1)
#else
#	define IS_WINDOWS (0)
#endif

#if defined(__linux__)
#	define IS_LINUX (1)
#else
#	define IS_LINUX (0)
#endif

#if defined(__APPLE__)
#	define IS_APPLE (1)
#else
#	define IS_APPLE (0)
#endif

#if defined(__unix__)
#	define IS_UNIX (1)
#else
#	define IS_UNIX (0)
#endif

	constexpr static inline bool IsWindows()
	{
		#if IS_WINDOWS 
			return true;
		#else
			return false;
		#endif
	}

	constexpr static inline bool IsApple()
	{
		#if IS_APPLE
			return true;
		#else
			return false;
		#endif
	}

	constexpr static inline bool IsLinux()
	{
		#if IS_LINUX
			return true;
		#else
			return false;
		#endif
	}

	constexpr static inline bool IsUnix()
	{
		#if IS_UNIX
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

	constexpr static inline bool IsRelease()
	{
		return !IsDebug();
	}

	constexpr static const char ENVIRONMENT_VARIABLE[] = "Environment";

	static inline bool IsEnvironment(const std::string& environmentName)
	{
		auto envValue = std::getenv(ENVIRONMENT_VARIABLE);
		if (envValue == nullptr) return false;

		return Utils::str_to_lower_copy(envValue) == 
			   Utils::str_to_lower_copy(environmentName);
	}

	static inline bool IsLocal()
	{
		return IsEnvironment("Local");
	}

	static inline bool IsDevelopment()
	{
		return IsEnvironment("Development");
	}

	static inline bool IsTesting()
	{
		return IsEnvironment("Testing");
	}

	static inline bool IsStaging()
	{
		return IsEnvironment("Staging");
	}

	static inline bool IsProduction()
	{
		return IsEnvironment("Production");
	}


#ifndef ARRAY_COUNT
#	define ARRAY_COUNT(array_) (sizeof(array_)/sizeof((array_)[0]))
#endif

#ifndef CSTR_COUNT
#	define CSTR_COUNT(cstr_) (ARRAY_COUNT(cstr_)-1)
#endif

}