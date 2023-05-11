#pragma once

#include "export.h"
#include <string>
#include "Step.h"

namespace OdbDesign::Lib
{
	class DECLSPEC OdbDesign
	{
	public:
		OdbDesign(std::string path);
		~OdbDesign();

		std::string GetPath() const;
		std::string GetProductName() const;

		const Step::StringMap& GetStepsByName() const;

		bool ParseDesign();

	private:
		std::string m_path;
		std::string m_productName;

		Step::StringMap m_stepsByName;

		bool ParseDesignDirectory(std::filesystem::path path);

	};
}
