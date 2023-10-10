#pragma once

#include <string>
#include "export.h"

namespace Odb::Lib
{
	class DECLSPEC IJsonConvertable
	{
	public:			
		virtual std::string to_json() const = 0;
		virtual void from_json(const std::string& json) = 0;

	protected:
		// abstract class/interface
		IJsonConvertable() = default;		

	};
}
