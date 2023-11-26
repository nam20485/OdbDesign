#include "FileArchive.h"
#include "FileArchive.h"
#include "FileArchive.h"
#include "FileArchive.h"
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
					loginfo("Successfully parsed. (" + s + " s)");

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
		
		for (const auto& kvStepDirectoryRecord : m_stepsByName)
		{
			(*pFileArchiveMessage->mutable_stepsbyname())[kvStepDirectoryRecord.first] = *kvStepDirectoryRecord.second->to_protobuf();
		}

		return pFileArchiveMessage;
	}

	void FileArchive::from_protobuf(const Odb::Lib::Protobuf::FileArchive& message)
	{
		m_matrixFile.from_protobuf(message.matrixfile());
		m_miscInfoFile.from_protobuf(message.miscinfofile());
		m_standardFontsFile.from_protobuf(message.standardfontsfile());

		for (const auto& kvStepDirectoryRecord : message.stepsbyname())
		{
			auto pStepDirectory = std::make_shared<StepDirectory>("");
			pStepDirectory->from_protobuf(kvStepDirectoryRecord.second);
			m_stepsByName[kvStepDirectoryRecord.first] = pStepDirectory;
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
}