#pragma once

#include <cstdlib>
#include "str_utils.h"


namespace Utils
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