#include "equals_within.h"

namespace Utils
{
	bool float_equals(float a, float b, float epsilon)
	{
		//return std::abs(a - b) < epsilon;
		return equals_within<float>(a, b, epsilon);
	}

	bool double_equals(double a, double b, double epsilon)
	{
		//return std::abs(a - b) < epsilon;
		return equals_within<double>(a, b, epsilon);
	}
}