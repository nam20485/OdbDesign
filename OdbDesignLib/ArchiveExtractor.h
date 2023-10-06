#pragma once

#include <string>
#include <filesystem>


class ArchiveExtractor
{
public:
	ArchiveExtractor(const std::string& path);
	//~ArchiveExtractor();	

	std::string GetPath() const;
	std::string GetExtractedPath() const;

	static bool IsArchiveTypeSupported(const std::filesystem::path& file);
	static bool IsArchiveTypeSupported(const std::string& file);

	bool Extract();
	bool Extract(const std::string& destinationPath);

	static inline const std::string SupportedExtensions[] = { "tgz", "tar.gz", "gz", "zip" };

private:
	std::string m_path;
	std::string m_extractedPath;	

};
