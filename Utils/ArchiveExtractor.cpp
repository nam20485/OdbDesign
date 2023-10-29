#include "ArchiveExtractor.h"
#include <filesystem>
#include "libarchive_extract.h"

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

	std::string ArchiveExtractor::GetExtractedPath() const
	{
		return m_extractedPath;
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
			auto command = "7z x " + m_path + " -o" + destinationPath + " -y";			
			auto exitCode = std::system(command.c_str());
			if (exitCode != 0) return false;
			m_extractedPath = destinationPath;
			return true;
		}
		else if (extract(m_path.c_str(), destinationPath.c_str()))
		{
			//std::filesystem::path p(destinationPath);
			//p /= std::filesystem::path(m_path).stem();
			m_extractedPath = destinationPath;
			return true;
		}

		return false;
	}
}