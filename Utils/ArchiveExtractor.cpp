#include "ArchiveExtractor.h"
#include <filesystem>
#include "libarchive_extract.h"
#include "Logger.h"
#include <exception>

using namespace std::filesystem;

namespace Utils
{
	ArchiveExtractor::ArchiveExtractor(const std::string& path)
		: m_path(path)
	{
	}

	std::string ArchiveExtractor::GetPath() const
	{
		return m_path;
	}

	std::string ArchiveExtractor::GetExtractionDirectory() const
	{
		return m_extractionDirectory;
	}

	bool ArchiveExtractor::IsArchiveTypeSupported(const std::filesystem::path& file)
	{
		return true;

		//for (const auto& ext : SupportedExtensions)
		//{
		//	if (file.extension() == "."+ext)
		//	{
		//		return true;
		//	}
		//}

		//return false;
	}

	bool ArchiveExtractor::IsArchiveTypeSupported(const std::string& file)
	{
		return IsArchiveTypeSupported(std::filesystem::path(file));
	}

	bool ArchiveExtractor::Extract()
	{
		auto path = std::filesystem::path(m_path);
		//auto extractionPath = path.replace_extension().string();
		auto extractionPath = path.parent_path() / path.stem();
		return Extract(extractionPath.string());
	}

	bool ArchiveExtractor::Extract(const std::string& destinationPath)
	{
		path p(m_path);
		if (p.extension() == ".Z" || p.extension() == ".z")
		{			
			// https://documentation.help/7-Zip/extract_full.htm

			std::stringstream ss;
			ss << "7z"
				<< " x " << '"' << m_path << '"'				// extract w/ full paths and archive path
				<< " -o" << '"' << destinationPath << '"'	// output path
				<< " -y" 									// yes to all prompts
				<< " -aoa";									// overwrite all

			const auto silent = false;
			if (silent)
			{
				ss << " > nul";
			}

			auto command = ss.str();

			loginfo("running 7z command: [" + command + "]...");

			auto exitCode = std::system(command.c_str());
			if (exitCode != (int) e7zExitCode::Success &&
				exitCode != (int) e7zExitCode::Warning)
			{
				auto message = "7z command failed (exit code = " + std::to_string(exitCode) + ")";
				logerror(message);
				throw std::exception(message.c_str());
				//return false;
			}

			loginfo("7z command succeeded");

			m_extractionDirectory = destinationPath;
			return true;
		}
		else if (extract(m_path.c_str(), destinationPath.c_str()))
		{
			//std::filesystem::path p(destinationPath);
			//p /= std::filesystem::path(m_path).stem();
			m_extractionDirectory = destinationPath;
			return true;
		}

		return false;
	}

	/*static*/ path ArchiveExtractor::getUncompressedFilePath(const path& directory, const std::string& filename)
	{
		path uncompressedPath;		
	
		path possibleCompressedFilePath = directory / filename;
		possibleCompressedFilePath.replace_extension("Z");

		auto compressedFileExists = false;
		if (exists(possibleCompressedFilePath) && is_regular_file(possibleCompressedFilePath))
		{
			compressedFileExists = true;
		}
		else
		{
			possibleCompressedFilePath.replace_extension("z");
			if (exists(possibleCompressedFilePath) && is_regular_file(possibleCompressedFilePath))
			{
				compressedFileExists = true;
			}
		}

		if (compressedFileExists)
		{
			// extract and set edaDataFilePath to file
			ArchiveExtractor extractor(possibleCompressedFilePath.string());
			if (extractor.Extract())
			{
				uncompressedPath = possibleCompressedFilePath;
				//uncompressedPath = extractor.GetExtractionDirectory();
				//uncompressedPath /= filename;
			}
		}
		else
		{
			path possibleUncompressedFilePath = directory / filename;
			auto uncompressedFileExists = exists(possibleUncompressedFilePath) && is_regular_file(possibleUncompressedFilePath);
			if (uncompressedFileExists)
			{
				uncompressedPath = possibleUncompressedFilePath;
			}
		}

		return uncompressedPath;
	}
}