#pragma once

#include <string>
#include "utils_export.h"

namespace Utils
{
	class UTILS_EXPORT IJsonable
	{
	public:		
		virtual ~IJsonable() {}

		virtual std::string to_json() const = 0;
		virtual void from_json(const std::string& json) = 0;

	protected:
		// abstract class/interface
		IJsonable() = default;		

	};
}
