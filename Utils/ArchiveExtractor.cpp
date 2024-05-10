#include "ArchiveExtractor.h"
#include "ArchiveExtractor.h"
#include <filesystem>
#include "libarchive_extract.h"
#include "Logger.h"
#include "str_utils.h"
#include <cstdlib>
#include <stdexcept>
#include <string>
#include "macros.h"
#include <sstream>

using namespace std::filesystem;

namespace Utils
{
	ArchiveExtractor::ArchiveExtractor(const std::string& path, bool bExtractLzmaInProc /* = false*/, bool bExtractLzmaUsing7Zip /* = true*/)
		: m_path(path)
		, m_bExtractLzmaInProc(bExtractLzmaInProc)
		, m_bExtractLzmaUsing7Zip(bExtractLzmaUsing7Zip)
	{
	}

	path ArchiveExtractor::GetPath() const
	{
		return m_path;
	}

	std::string ArchiveExtractor::GetExtractionDirectory() const
	{
		return m_extractionDirectory;
	}

	bool ArchiveExtractor::IsArchiveTypeSupported(const path& file)
	{
		if (ALLOW_ALL_ARCHIVE_EXTENSION_TYPES) return true;

		for (const auto& ext : SupportedExtensions)
		{
			auto extLower = str_to_lower_copy(file.extension().string());
			if (extLower == "." + std::string(ext))
			{
				return true;
			}
		}		

		return false;
	}

	bool ArchiveExtractor::IsArchiveTypeSupported(const std::string& file)
	{
		return IsArchiveTypeSupported(path(file));
	}

	bool ArchiveExtractor::Extract()
	{
		//auto extractionPath = path.replace_extension().string();
		auto extractionPath = m_path.parent_path() / m_path.stem();
		return Extract(extractionPath.string());
	}

	bool Utils::ArchiveExtractor::ExtractLzmaInProc(const std::filesystem::path& destinationPath)
	{
		return false;
	}

	bool ArchiveExtractor::ExtractLzmaOutOfProc(const path& destinationPath)
	{
		// https://documentation.help/7-Zip/extract_full.htm

		std::stringstream ss;
		ss << "7z"
			<< " x " /*<< '"'*/ << m_path /*<< '"'*/			// extract w/ full paths and archive path
			<< " -o" /*<< '"'*/ << destinationPath /*<< '"'*/	// output path
			<< " -y" 											// yes to all prompts
			<< " -aoa";											// overwrite all

		if (HIDE_7Z_COMMAND_OUTPUT)
		{
			if (Utils::IsWindows())
			{
				ss << " >$null 2>&1";
			}
			else
			{
				ss << " >nul 2>nul";
			}
		}

		auto command = ss.str();

		loginfo("running 7z command: [" + command + "]...");

		auto exitCode = std::system(command.c_str());

#if !IS_WINDOWS
		exitCode = WEXITSTATUS(exitCode);
#endif

		if (exitCode != static_cast<int>(e7zExitCode::Success) &&
			exitCode != static_cast<int>(e7zExitCode::Warning))
		{
			auto message = "7z command failed (exit code = " + std::to_string(exitCode) + ")";
			message += ", cwd: [" + current_path().string() + "]";
			logerror(message);
			throw std::runtime_error(message.c_str());
			//return false;
		}

		loginfo("7z command succeeded");

		m_extractionDirectory = destinationPath.string();
		return true;
	}

	bool ArchiveExtractor::Extract(const std::string& destinationPath)
	{
		if (m_bExtractLzmaUsing7Zip && 
			std::find(LZMA_FILE_EXTENSIONS.begin(), LZMA_FILE_EXTENSIONS.end(), m_path.extension()) != LZMA_FILE_EXTENSIONS.end())
		{			
			if (m_bExtractLzmaInProc)
			{
				return ExtractLzmaInProc(destinationPath);
			}
			else
			{
				return ExtractLzmaOutOfProc(destinationPath);
			}			
		}
		else if (extract(m_path.string().c_str(), destinationPath.c_str()))
		{
			//path p(destinationPath);
			//p /= path(m_path).stem();
			m_extractionDirectory = destinationPath;
			return true;
		}

		return false;
	}

	/*static*/ path ArchiveExtractor::getUncompressedFilePath(const path& directory, const std::string& filename)
	{
		path uncompressedPath;		

		path possibleUncompressedFilePath = directory / filename;
		auto uncompressedFileExists = exists(possibleUncompressedFilePath) && is_regular_file(possibleUncompressedFilePath);
		if (uncompressedFileExists)
		{
			uncompressedPath = possibleUncompressedFilePath;
		}
		else
		{
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
					uncompressedPath = extractor.GetExtractionDirectory();
					uncompressedPath /= filename;
				}
			}
		}		

		return uncompressedPath;
	}

	/*static*/ bool ArchiveExtractor::CompressDir(const std::string& srcDir, const std::string& destDir, 
												  const std::string& archiveName, std::string& fileOut, 
												  CompressionType type)
	{
		return compress_dir(srcDir.c_str(), destDir.c_str(), archiveName.c_str(), fileOut, type);
	}
}