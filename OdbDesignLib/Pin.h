#pragma once

#include "export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>


namespace OdbDesign::Lib::Design
{
	class DECLSPEC Pin
	{
	public:
		Pin();
		~Pin();

		std::string GetName() const;

		typedef std::vector<std::shared_ptr<Pin>> Vector;
		typedef std::map<std::string, std::shared_ptr<Pin>> StringMap;

	private:
		std::string m_name;

	};
} // namespace OdbDesign::Lib::Design
