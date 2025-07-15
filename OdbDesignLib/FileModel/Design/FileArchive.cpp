#include "FileArchive.h"
#include <filesystem>
#include "ArchiveExtractor.h"
#include "MiscInfoFile.h"
#include "Logger.h"
#include "StopWatch.h"
#include "fastmove.h"
#include <system_error>
#include <cstdio>
#include <string>
#include <memory>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::FileModel::Design
{
	FileArchive::FileArchive()		
		: m_filePath()
	{			
	}

	FileArchive::FileArchive(const std::string& path)
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
				path extractedPath;
				if (!ExtractDesignArchive(m_filePath, extractedPath))
				{
					logerror("failed to extract archive: (" + m_filePath + ")");
					return false;
				}

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

	bool FileArchive::SaveFileModel(const path& directory)
	{
		return SaveFileModel(directory, m_productName);	
	}

	bool FileArchive::SaveFileModel(const path& directory, const std::string& archiveName)
	{
		// create directory in /tmp
		// write dir structure and files to it
		// gzip with archiveName
		// move archive to directory

		char szTmpNameBuff[L_tmpnam] = { 0 };
		if (nullptr == std::tmpnam(szTmpNameBuff)) return false;
		
		auto tempPath = temp_directory_path() / szTmpNameBuff;
		if (!create_directory(tempPath)) return false;

		auto rootPath = tempPath / archiveName;
		if (!create_directory(rootPath)) return false;
		if (!Save(rootPath)) return false;

		// compress the written file structure
		std::string createdArchivePath;
		if (! Utils::ArchiveExtractor::CompressDir(rootPath.string(), tempPath.string(), archiveName, createdArchivePath)) return false;
		if (createdArchivePath.empty()) return false;

		// move the compressed file to the requested save directory
		path archiveFilename = path(createdArchivePath).filename();
		path destPath = directory / archiveFilename;
		std::error_code ec;
		Utils::fastmove_file(createdArchivePath, destPath, true, ec);
		if (ec.value() != 0) return false;		

		return true;
	}	

	bool FileArchive::Save(const path& directory)
	{
		// MiscInfoFile
		auto miscPath = directory / "misc";
		if (!create_directory(miscPath)) return false;

		std::ofstream ofs1(miscPath / "info", std::ios::out);
		if (!m_miscInfoFile.Save(ofs1)) return false;
		ofs1.close();

		// MiscAttrFile
		std::ofstream ofs2(miscPath / "sysattr");
		if (!m_miscAttrListFile.Save(ofs2)) return false;
		ofs2.close();

		// StandardFontsFile
		auto fontsPath = directory / "fonts";
		if (!create_directory(fontsPath)) return false;
		std::ofstream ofs3(fontsPath / "standard");
		if (!m_standardFontsFile.Save(ofs3)) return false;
		ofs3.close();

		// MatrixFile		
		auto matrixPath = directory / "matrix";
		if (!create_directory(matrixPath)) return false;
		std::ofstream ofs4(matrixPath / "matrix");
		if (!m_matrixFile.Save(ofs4)) return false;
		ofs4.close();

		// Steps
		const auto stepsDirectory = directory / "steps";	
		if (!create_directory(stepsDirectory)) return false;
		for (auto& kvStepDirectory : m_stepsByName)
		{
			if (!kvStepDirectory.second->Save(stepsDirectory)) return false;
		}

		// Symbols
		const auto symbolsDirectory = directory / "symbols";
		if (!create_directory(symbolsDirectory)) return false;
		for (auto& kvSymbolsDirectory : m_symbolsDirectoriesByName)
		{
			if (!kvSymbolsDirectory.second->Save(symbolsDirectory)) return false;
		}

		return true;
	}

	bool FileArchive::ExtractDesignArchive(const path& archivePath, path& extractedPath)
	{
		loginfo("Extracting... ");

		if (!Utils::ArchiveExtractor::IsArchiveTypeSupported(archivePath))
		{
			logerror("Unsupported archive type: (" + archivePath.string() + ")");
			return false;			
		}

		Utils::ArchiveExtractor extractor(archivePath.string());
		if (!extractor.Extract()) return false;

		auto extracted = path(extractor.GetExtractionDirectory());
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

	/*static*/ bool FileArchive::pathContainsTopLevelDesignDirs(const path& path)
	{
		for (const auto& topLevelRootDirName : TOPLEVEL_DESIGN_DIR_NAMES)
		{
			auto rootLevelDirPath = path / topLevelRootDirName;
			if (!exists(rootLevelDirPath)) return false;
		}
		return true;
	}

	std::unique_ptr<Protobuf::FileArchive> FileArchive::to_protobuf() const
	{
		std::unique_ptr<Protobuf::FileArchive> pFileArchiveMessage(new Protobuf::FileArchive);
		pFileArchiveMessage->set_productname(m_productName);
		pFileArchiveMessage->set_filename(m_filename);
		//pFileArchiveMessage->set_filepath(m_filePath);
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

	void FileArchive::from_protobuf(const Protobuf::FileArchive& message)
	{
		m_productName = message.productname();
		m_filename = message.filename();
		//m_filePath = message.filepath();
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

	bool FileArchive::ParseDesignDirectory(const path& path)
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

	bool FileArchive::ParseStepDirectories(const path& path)
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

	bool FileArchive::ParseMiscAttrListFile(const path& path)
	{
		loginfo("Parsing misc/attrlist file...");

		auto miscDirectory = path / "misc";
		if (!exists(miscDirectory)) return false;
		if (!is_directory(miscDirectory)) return false;

		if (!m_miscAttrListFile.Parse(miscDirectory)) return false;

		loginfo("Parsing misc/attrlist file complete");

		return true;
	}

	bool FileArchive::ParseMatrixFile(const path& path)
	{
		loginfo("Parsing matrix/matrix file...");

		auto matrixDir = path / "matrix";
		if (!exists(matrixDir)) return false;
		if (!is_directory(matrixDir)) return false;

		if (!m_matrixFile.Parse(matrixDir)) return false;

		loginfo("Parsing matrix/matrix file complete");

		return true;
	}
	bool FileArchive::ParseStandardFontsFile(const path& path)
	{
		loginfo("Parsing fonts/standard file...");

		auto fontsDir = path / "fonts";
		if (!exists(fontsDir)) return false;
		if (!is_directory(fontsDir)) return false;

		if (!m_standardFontsFile.Parse(fontsDir)) return false;

		loginfo("Parsing fonts/standard file complete");

		return true;
	}

	bool FileArchive::ParseSymbolsDirectories(const path& path)
	{
		loginfo("Parsing symbols root directory...");

		auto symbolsDirectory = path / "symbols";

		if (!exists(symbolsDirectory))
		{
			logwarn("symbols root directory does not exist (" + symbolsDirectory.string() + ")");
			return true;
		}
		else if (!is_directory(symbolsDirectory))
		{
			logerror("symbols root directory exists but is a regular file/not a directory (" + symbolsDirectory.string() + ")");
			return false;
		}

		for (auto& d : directory_iterator(symbolsDirectory))
		{
			if (is_directory(d))
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

	std::shared_ptr<StepDirectory> FileArchive::GetStepDirectory(const std::string& stepName /*= ""*/) const
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