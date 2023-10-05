#include "ArchiveExtractor.h"
#include <filesystem>
#include "libarchive_extract.h"


ArchiveExtractor::ArchiveExtractor(const std::string& path)
	: m_path(path)
{
}

std::string ArchiveExtractor::GetPath() const
{
	return m_path;
}

std::string ArchiveExtractor::GetExtractedPath() const
{
	return m_extractedPath;
}

bool ArchiveExtractor::IsArchiveTypeSupported(const std::filesystem::path& file)
{
	for (const auto& ext : SupportedExtensions)
	{
		if (file.extension() == "."+ext)
		{
			return true;
		}
	}

	return false;
}

bool ArchiveExtractor::IsArchiveTypeSupported(const std::string& file)
{
	return IsArchiveTypeSupported(std::filesystem::path(file));
}

bool ArchiveExtractor::Extract()
{
	auto path = std::filesystem::path(m_path);
	auto extractionPath = path.replace_extension().string();
	return Extract(extractionPath);
}

bool ArchiveExtractor::Extract(const std::string& destinationPath)
{
	if (extract(m_path.c_str()))
	{
		m_extractedPath = destinationPath;
		return true;
	}
		
	return false;
}
