#pragma once

#include <string>
#include "export.h"

namespace Odb::Lib
{
	class DECLSPEC IJsonConvertable
	{
	public:			
		virtual std::string to_json() const = 0;
		//void from_json(const std::string& json);
		//static TMessage* from_json(const std::string& json);

	protected:
		// abstract class/interface
		IJsonConvertable() = default;		

	};
}
