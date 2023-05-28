#pragma once

#include "export.h"
#include <string>
#include "StepDirectory.h"

namespace Odb::Lib::FileModel::Design
{
	class DECLSPEC FileModel
	{
	public:
		FileModel(std::string path);
		~FileModel();

		std::string GetPath() const;
		std::string GetProductName() const;

		const StepDirectory::StringMap& GetStepsByName() const;

		bool ParseFileModel();

	private:
		std::string m_path;
		std::string m_productName;

		StepDirectory::StringMap m_stepsByName;

		bool ParseDesignDirectory(std::filesystem::path path);

	};
}
