#pragma once

#include "../../win.h"
#include "../../odbdesign_export.h"
#include <string>
#include "StepDirectory.h"
#include "EdaDataFile.h"
#include <map>
#include <vector>
#include "MiscInfoFile.h"
#include "MatrixFile.h"
#include "StandardFontsFile.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT FileArchive
	{
	public:
		FileArchive(std::string path);
		~FileArchive();

		std::string GetPath() const;
		std::string GetProductName() const;
		std::string GetFilename() const;

		const StepDirectory::StringMap& GetStepsByName() const;
        const MiscInfoFile& GetMiscInfoFile() const;
		const MatrixFile& GetMatrixFile() const;
		const StandardFontsFile& GetStandardFontsFile() const;

		// TODO: fix these to use pointer return types
		//const EdaDataFile& GetStepEdaDataFile(std::string stepName) const;
		//const EdaDataFile& GetFirstStepEdaDataFile() const;		

		bool ParseFileModel();

		typedef std::vector<std::shared_ptr<FileArchive>> Vector;
		typedef std::map<std::string, std::shared_ptr<FileArchive>> StringMap;

	private:
		std::string m_path;
		std::string m_productName;

		StepDirectory::StringMap m_stepsByName;
        MiscInfoFile m_miscInfoFile;
		MatrixFile m_matrixFile;
		StandardFontsFile m_standardFontsFile;

		bool ParseDesignDirectory(const std::filesystem::path& path);
        bool ParseMiscInfoFile(const std::filesystem::path& path);
		bool ParseMatrixFile(const std::filesystem::path& path);
		bool ParseStandardFontsFile(const std::filesystem::path& path);

		static bool ExtractDesignArchive(const std::filesystem::path& path, std::filesystem::path& extractedPath);

    };
}