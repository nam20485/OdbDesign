#pragma once

#include <cmath>
#include "utils_export.h"

namespace Utils
{
	template<class R>
	bool equals_within(const R a, const R b, const R epsilon = 0.001f)
	{
		return std::abs(a - b) < epsilon;
	}

	UTILS_EXPORT bool float_equals(float a, float b, float epsilon = 0.001f);

	UTILS_EXPORT bool double_equals(double a, double b, double epsilon = 0.001);
}