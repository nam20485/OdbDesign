#pragma once

#include <string>
#include <filesystem>
#include "utils_export.h"
#include "libarchive_extract.h"
#include <vector>


namespace Utils
{
	class UTILS_EXPORT ArchiveExtractor
	{
	public:

		enum class e7zExitCode
		{
			Success = 0,
			Warning = 1,
			FatalError = 2,
			CommandLineError = 7,
			NotEnoughMemory = 8,
			UserStopped = 255
		};

		ArchiveExtractor(const std::string& path, bool bExtractLzmaInProc = false, bool bExtractLzmaUsing7Zip = true);
		//~ArchiveExtractor();	

		std::filesystem::path GetPath() const;
		std::string GetExtractionDirectory() const;

		static bool IsArchiveTypeSupported(const std::filesystem::path& file);
		static bool IsArchiveTypeSupported(const std::string& file);

		bool Extract();
		bool Extract(const std::string& destinationPath);

		static std::filesystem::path getUncompressedFilePath(const std::filesystem::path& directory, const std::string& filename);

		static bool CompressDir(const std::string& srcDir, const std::string& destDir, 
								const std::string& archiveName, std::string& fileOut, 
								CompressionType type = CompressionType::TarGzip);

		constexpr static inline bool ALLOW_ALL_ARCHIVE_EXTENSION_TYPES = false;
		constexpr static inline const char* SupportedExtensions[] = { "tgz", "tar.gz", "gz", "zip", "Z", "gzip", "tar" };

	private:
		std::filesystem::path m_path;
		std::string m_extractionDirectory;
		bool m_bExtractLzmaInProc = false;
		bool m_bExtractLzmaUsing7Zip = true;

		bool ExtractLzmaOutOfProc(const std::filesystem::path& destinationPath);
		bool ExtractLzmaInProc(const std::filesystem::path& destinationPath);

		static inline const std::vector<std::string> LZMA_FILE_EXTENSIONS = { ".Z", ".z", ".lzma", ".lz" };

		constexpr static inline bool HIDE_7Z_COMMAND_OUTPUT = true;

	};
}
