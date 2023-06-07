#pragma once

#include "export.h"
#include <string>
#include "StepDirectory.h"
#include "EdaDataFile.h"


namespace Odb::Lib::FileModel::Design
{
	class DECLSPEC FileArchive
	{
	public:
		FileArchive(std::string path);
		~FileArchive();

		std::string GetPath() const;
		std::string GetProductName() const;

		const StepDirectory::StringMap& GetStepsByName() const;		

		// TODO: fix these to use pointer return types
		//const EdaDataFile& GetStepEdaDataFile(std::string stepName) const;
		//const EdaDataFile& GetFirstStepEdaDataFile() const;

		bool ParseFileModel();

	private:
		std::string m_path;
		std::string m_productName;

		StepDirectory::StringMap m_stepsByName;

		bool ParseDesignDirectory(std::filesystem::path path);

	};
}
