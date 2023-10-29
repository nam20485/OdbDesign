#include "FileArchive.h"
#include "FileArchive.h"
#include <filesystem>
#include "ArchiveExtractor.h"
#include "MiscInfoFile.h"
#include <iostream>
#include "Logger.h"

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::FileModel::Design
{

	FileArchive::FileArchive(std::string path)
		: m_filePath(path)
	{
	}

	FileArchive::~FileArchive()
	{
	}

	std::string FileArchive::GetRootDir() const
	{
		return m_rootDir;
	}

	std::string FileArchive::GetProductName() const
	{
		return m_productName;
	}

	std::string FileArchive::GetFilePath() const
	{
		return m_filePath;
	}

	std::string FileArchive::GetFilename() const
	{
		//return m_filename;
		return path(m_filePath).filename().string();
	}

	const StepDirectory::StringMap& FileArchive::GetStepsByName() const
	{ 
		return m_stepsByName;
	}

	bool FileArchive::ParseFileModel()
	{
		try
		{
			if (!exists(m_filePath)) return false;
		
			if (is_regular_file(m_filePath))
			{
				std::filesystem::path extractedPath;
				if (! ExtractDesignArchive(m_filePath, extractedPath)) return false;

				m_rootDir = findRootDir(extractedPath);
			}			
		
			if (is_directory(m_rootDir))
			{
				loginfo("Parsing... ");

				if (ParseDesignDirectory(m_rootDir))
				{
					loginfo("Successfully parsed.");
					return true;
				}
				else
				{
					loginfo("Parsing failed.");
				}
			}		
		}		
		catch (std::filesystem::filesystem_error& fe)
		{
			logexception(fe);
		}
		catch (std::exception& e)
		{
			logexception(e);
		}

		return false;
	}

	bool FileArchive::ExtractDesignArchive(const std::filesystem::path& path, std::filesystem::path& extractedPath)
	{
		loginfo("Extracting... ");

		if (!Utils::ArchiveExtractor::IsArchiveTypeSupported(path)) return false;

		Utils::ArchiveExtractor extractor(path.string());
		if (!extractor.Extract()) return false;

		auto extracted = std::filesystem::path(extractor.GetExtractedPath());
		if (!exists(extracted)) return false;

		extractedPath = extracted;

		loginfo("Successfully extracted.");

		return true;
				
	}

	/*static*/ std::string FileArchive::findRootDir(const path& extractedPath)
	{
		if (pathContainsTopLevelDesignDirs(extractedPath))
		{
			return extractedPath.string();
		}
		else
		{
			for (const auto& p : directory_iterator(extractedPath))
			{
				if (is_directory(p))
				{
					if (pathContainsTopLevelDesignDirs(p.path()))
					{
						return p.path().string();
					}
				}
			}

			return "";
		}
	}

	/*static*/ bool FileArchive::pathContainsTopLevelDesignDirs(const std::filesystem::path& path)
	{
		for (const auto& topLevelRootDirName : TOPLEVEL_DESIGN_DIR_NAMES)
		{
			auto rootLevelDirPath = path / topLevelRootDirName;
			if (!exists(rootLevelDirPath)) return false;
		}
		return true;
	}

	bool FileArchive::ParseDesignDirectory(const std::filesystem::path& path)
	{
		if (!exists(path)) return false;
		else if (!is_directory(path)) return false;

		m_productName = path.stem().string();

		auto stepsPath = path / "steps";
		for (auto& d : directory_iterator(stepsPath))
		{
			if (is_directory(d))
			{
				auto pStep = std::make_shared<StepDirectory>(d.path());
				if (pStep->Parse())
				{
					m_stepsByName[pStep->GetName()] = pStep;
				}
				else
				{
					return false;
				}
			}
		}

        if (! ParseMiscInfoFile(path)) return false;
		if (! ParseMatrixFile(path)) return false;
		if (! ParseStandardFontsFile(path)) return false;

		return true;
	}

    bool FileArchive::ParseMiscInfoFile(const path& path)
    {
        auto miscDirectory = path / "misc";
        if (!exists(miscDirectory)) return false;
        if (!is_directory(miscDirectory)) return false;

        if (!m_miscInfoFile.Parse(miscDirectory)) return false;

        return true;
    }

	bool FileArchive::ParseMatrixFile(const std::filesystem::path& path)
	{
		auto matrixDir = path / "matrix";
		if (!exists(matrixDir)) return false;
		if (!is_directory(matrixDir)) return false;

		if (!m_matrixFile.Parse(matrixDir)) return false;

		return true;
	}
	bool FileArchive::ParseStandardFontsFile(const std::filesystem::path& path)
	{
		auto fontsDir = path / "fonts";
		if (!exists(fontsDir)) return false;
		if (!is_directory(fontsDir)) return false;

		if (!m_standardFontsFile.Parse(fontsDir)) return false;

		return true;
	}

    const MiscInfoFile &FileArchive::GetMiscInfoFile() const
    {
        return m_miscInfoFile;
    }

	const MatrixFile& FileArchive::GetMatrixFile() const
	{
		return m_matrixFile;
	}

	const StandardFontsFile& FileArchive::GetStandardFontsFile() const
	{
		return m_standardFontsFile;
	}

    //const EdaDataFile& FileModel::GetStepEdaDataFile(std::string stepName) const
	//{		
	//	auto findIt = m_stepsByName.find(stepName);
	//	if (findIt != m_stepsByName.end())
	//	{
	//		return findIt->second->GetEdaDataFile();
	//	}
	//	else
	//	{
	//		return EdaDataFile::EMPTY;
	//	}		
	//}

	//const EdaDataFile& FileModel::GetFirstStepEdaDataFile() const
	//{
	//	if (!m_stepsByName.empty())
	//	{
	//		return m_stepsByName.begin()->second->GetEdaDataFile();
	//	}
	//	else
	//	{
	//		return EdaDataFile::EMPTY;
	//	}
	//}
}