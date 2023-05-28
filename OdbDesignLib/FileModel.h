#pragma once

#include "export.h"
#include <string>
#include "Step.h"

namespace OdbDesign::Lib::FileModel
{
	class DECLSPEC FileModel
	{
	public:
		FileModel(std::string path);
		~FileModel();

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
