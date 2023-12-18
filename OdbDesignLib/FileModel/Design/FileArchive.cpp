#include "FileArchive.h"
#include <filesystem>
#include "ArchiveExtractor.h"
#include "MiscInfoFile.h"
#include <iostream>
#include "Logger.h"
#include "StopWatch.h"

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
		m_stepsByName.clear();
		m_symbolsDirectoriesByName.clear();
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

	const SymbolsDirectory::StringMap& FileArchive::GetSymbolsDirectoriesByName() const
	{
		return m_symbolsDirectoriesByName;
	}

	bool FileArchive::ParseFileModel()
	{
		//try
		{
			StopWatch timer(true);

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
					timer.stop();					
					auto s = timer.getElapsedSecondsString();
					loginfo("Successfully parsed. (" + s + "s)");

					return true;
				}
				else
				{
					logerror("Parsing failed.");
					throw std::runtime_error("Parsing failed.");
				}
			}
			else
			{
				logerror("Failed to find root directory");
			}
		}
		//catch (std::exception& e)
		//{
		//	logexception(e);
		//	throw e;
		//}

		return false;
	}

	bool FileArchive::ExtractDesignArchive(const std::filesystem::path& path, std::filesystem::path& extractedPath)
	{
		loginfo("Extracting... ");

		if (!Utils::ArchiveExtractor::IsArchiveTypeSupported(path)) return false;

		Utils::ArchiveExtractor extractor(path.string());
		if (!extractor.Extract()) return false;

		auto extracted = std::filesystem::path(extractor.GetExtractionDirectory());
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

	std::unique_ptr<Odb::Lib::Protobuf::FileArchive> FileArchive::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::FileArchive> pFileArchiveMessage(new Odb::Lib::Protobuf::FileArchive);
		pFileArchiveMessage->mutable_matrixfile()->CopyFrom(*m_matrixFile.to_protobuf());
		pFileArchiveMessage->mutable_miscinfofile()->CopyFrom(*m_miscInfoFile.to_protobuf());
		pFileArchiveMessage->mutable_standardfontsfile()->CopyFrom(*m_standardFontsFile.to_protobuf());
		pFileArchiveMessage->mutable_miscattrlistfile()->CopyFrom(*m_miscAttrListFile.to_protobuf());
		
		for (const auto& kvStepDirectoryRecord : m_stepsByName)
		{
			(*pFileArchiveMessage->mutable_stepsbyname())[kvStepDirectoryRecord.first] = *kvStepDirectoryRecord.second->to_protobuf();
		}

		for (const auto& kvSymbolsDirectory : m_symbolsDirectoriesByName)
		{
			(*pFileArchiveMessage->mutable_symbolsdirectoriesbyname())[kvSymbolsDirectory.first] = *kvSymbolsDirectory.second->to_protobuf();
		}

		return pFileArchiveMessage;
	}

	void FileArchive::from_protobuf(const Odb::Lib::Protobuf::FileArchive& message)
	{
		m_matrixFile.from_protobuf(message.matrixfile());
		m_miscInfoFile.from_protobuf(message.miscinfofile());
		m_standardFontsFile.from_protobuf(message.standardfontsfile());
		m_miscAttrListFile.from_protobuf(message.miscattrlistfile());

		for (const auto& kvStepDirectoryRecord : message.stepsbyname())
		{
			auto pStepDirectory = std::make_shared<StepDirectory>("");
			pStepDirectory->from_protobuf(kvStepDirectoryRecord.second);
			m_stepsByName[kvStepDirectoryRecord.first] = pStepDirectory;
		}

		for (const auto& kvSymbolsDirectory : message.symbolsdirectoriesbyname())
		{
			auto pSymbolsDirectory = std::make_shared<SymbolsDirectory>("");
			pSymbolsDirectory->from_protobuf(kvSymbolsDirectory.second);
			m_symbolsDirectoriesByName[kvSymbolsDirectory.first] = pSymbolsDirectory;
		}
	}

	bool FileArchive::ParseDesignDirectory(const std::filesystem::path& path)
	{
		if (!exists(path)) return false;
		else if (!is_directory(path)) return false;

		m_productName = path.stem().string();

		if (! ParseStepDirectories(path)) return false;
        if (! ParseMiscInfoFile(path)) return false;
		if (! ParseMatrixFile(path)) return false;
		if (! ParseStandardFontsFile(path)) return false;
		if (! ParseSymbolsDirectories(path)) return false;
		if (! ParseMiscAttrListFile(path)) return false;

		return true;
	}

	bool FileArchive::ParseStepDirectories(const std::filesystem::path& path)
	{
		loginfo("Parsing steps...");

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
					logwarn("Failed to parse step: " + pStep->GetName());
					//return false;
				}
			}
		}

		loginfo("Parsing steps complete");

		return true;
	}

    bool FileArchive::ParseMiscInfoFile(const path& path)
    {
		loginfo("Parsing misc/info file...");

        auto miscDirectory = path / "misc";
        if (!exists(miscDirectory)) return false;
        if (!is_directory(miscDirectory)) return false;

        if (!m_miscInfoFile.Parse(miscDirectory)) return false;

		loginfo("Parsing misc/info file complete");

        return true;
    }

	bool FileArchive::ParseMiscAttrListFile(const std::filesystem::path& path)
	{
		loginfo("Parsing misc/attrlist file...");

		auto miscDirectory = path / "misc";
		if (!exists(miscDirectory)) return false;
		if (!is_directory(miscDirectory)) return false;

		if (!m_miscAttrListFile.Parse(miscDirectory)) return false;

		loginfo("Parsing misc/attrlist file complete");

		return true;
	}

	bool FileArchive::ParseMatrixFile(const std::filesystem::path& path)
	{
		loginfo("Parsing matrix/matrix file...");

		auto matrixDir = path / "matrix";
		if (!exists(matrixDir)) return false;
		if (!is_directory(matrixDir)) return false;

		if (!m_matrixFile.Parse(matrixDir)) return false;

		loginfo("Parsing matrix/matrix file complete");

		return true;
	}
	bool FileArchive::ParseStandardFontsFile(const std::filesystem::path& path)
	{
		loginfo("Parsing fonts/standard file...");

		auto fontsDir = path / "fonts";
		if (!exists(fontsDir)) return false;
		if (!is_directory(fontsDir)) return false;

		if (!m_standardFontsFile.Parse(fontsDir)) return false;

		loginfo("Parsing fonts/standard file complete");

		return true;
	}

	bool FileArchive::ParseSymbolsDirectories(const std::filesystem::path& path)
	{
		loginfo("Parsing symbols root directory...");

		auto symbolsDirectory = path / "symbols";

		if (!std::filesystem::exists(symbolsDirectory))
		{
			logwarn("symbols root directory does not exist (" + symbolsDirectory.string() + ")");
			return true;
		}
		else if (!std::filesystem::is_directory(symbolsDirectory))
		{
			logerror("symbols root directory exists but is a regular file/not a directory (" + symbolsDirectory.string() + ")");
			return false;
		}

		for (auto& d : std::filesystem::directory_iterator(symbolsDirectory))
		{
			if (std::filesystem::is_directory(d))
			{
				auto pSymbolsDirectory = std::make_shared<SymbolsDirectory>(d.path());
				if (pSymbolsDirectory->Parse())
				{
					//loginfo("Parsing symbols directory: " + pSymbolsDirectory->GetName() + " complete");

					m_symbolsDirectoriesByName[pSymbolsDirectory->GetName()] = pSymbolsDirectory;
				}
				else
				{
					logerror("Parsing symbol directory: " + pSymbolsDirectory->GetName() + " failed");
					return false;
				}
			}
		}

		loginfo("Parsing symbols root directory complete");

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

	const AttrListFile& FileArchive::GetMiscAttrListFile() const
	{
		return m_miscAttrListFile;
	}

	std::shared_ptr<StepDirectory> FileArchive::GetStepDirectory(const std::string& stepName /*= ""*/)
	{
		std::shared_ptr<FileModel::Design::StepDirectory> pStepDirectory;

		const auto& steps = GetStepsByName();
		if (!steps.empty())
		{
			if (stepName.empty())
			{
				// return first step
				pStepDirectory = steps.begin()->second;
			}
			else
			{
				auto findIt = steps.find(stepName);
				if (findIt != steps.end())
				{
					pStepDirectory = findIt->second;
				}
			}
		}		

		return pStepDirectory;
	}
}