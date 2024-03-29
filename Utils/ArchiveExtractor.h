#pragma once

#include <string>
#include <filesystem>
#include "utils_export.h"


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

		ArchiveExtractor(const std::string& path);
		//~ArchiveExtractor();	

		std::string GetPath() const;
		std::string GetExtractionDirectory() const;

		static bool IsArchiveTypeSupported(const std::filesystem::path& file);
		static bool IsArchiveTypeSupported(const std::string& file);

		bool Extract();
		bool Extract(const std::string& destinationPath);

		static std::filesystem::path getUncompressedFilePath(const std::filesystem::path& directory, const std::string& filename);

		inline static bool ALLOW_ALL_ARCHIVE_EXTENSION_TYPES = false;
		constexpr static inline const char* SupportedExtensions[] = { "tgz", "tar.gz", "gz", "zip", "Z", "gzip", "tar" };

	private:
		std::string m_path;
		std::string m_extractionDirectory;

	};
}
