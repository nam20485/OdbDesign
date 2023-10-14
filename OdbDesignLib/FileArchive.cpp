#include "FileArchive.h"
#include <filesystem>
#include "ArchiveExtractor.h"
#include "MiscInfoFile.h"
#include <iostream>


namespace Odb::Lib::FileModel::Design
{

	FileArchive::FileArchive(std::string path)
		: m_path(path)
	{
	}

	FileArchive::~FileArchive()
	{
	}

	std::string FileArchive::GetPath() const
	{
		return m_path;
	}

	std::string FileArchive::GetProductName() const
	{
		return m_productName;
	}

	const StepDirectory::StringMap& FileArchive::GetStepsByName() const { return m_stepsByName; }

	bool FileArchive::ParseFileModel()
	{
		try
		{
			std::filesystem::path path(m_path);

			if (!std::filesystem::exists(path)) return false;
		
			if (std::filesystem::is_regular_file(path))
			{
				std::filesystem::path extractedPath;
				if (! ExtractDesignArchive(path, extractedPath)) return false;
				path = extractedPath;
			}			
		
			if (std::filesystem::is_directory(path))
			{
				std::cout << " - Parsing... ";

				if (ParseDesignDirectory(path))
				{
					std::cout << "success." << std::endl << std::endl;
					return true;
				}
			}		
		}		
		catch (std::filesystem::filesystem_error& fe)
		{
			std::cout << fe.what() << std::endl;
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}

		return false;
	}

	bool FileArchive::ExtractDesignArchive(const std::filesystem::path& path, std::filesystem::path& extractedPath)
	{
		std::cout << " - Extracting... ";

		if (!Utils::ArchiveExtractor::IsArchiveTypeSupported(path)) return false;

		Utils::ArchiveExtractor extractor(path.string());
		if (!extractor.Extract()) return false;

		auto extracted = std::filesystem::path(extractor.GetExtractedPath());
		if (!std::filesystem::exists(extracted)) return false;
		
		extractedPath = extracted;

		std::cout << "success." << std::endl;

		return true;
				
	}

	bool FileArchive::ParseDesignDirectory(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path)) return false;
		else if (!std::filesystem::is_directory(path)) return false;

		m_productName = path.filename().string();

		auto stepsPath = path / "steps";
		for (auto& d : std::filesystem::directory_iterator(stepsPath))
		{
			if (std::filesystem::is_directory(d))
			{
				auto pStep = std::make_shared<StepDirectory>(d.path());
				if (pStep->Parse())
				{
					m_stepsByName[pStep->GetName()] = pStep;
				}
				else
				{
					return false;
				}
			}
		}

        if (! ParseMiscInfoFile(path)) return false;

		return true;
	}

    bool FileArchive::ParseMiscInfoFile(const std::filesystem::path& path)
    {
        auto miscDirectory = path / "misc";
        if (!std::filesystem::exists(miscDirectory)) return false;
        if (!std::filesystem::is_directory(miscDirectory)) return false;

        if (!m_miscInfoFile.Parse(miscDirectory)) return false;

        return true;
    }

    const MiscInfoFile &FileArchive::GetMiscInfoFile() const
    {
        return m_miscInfoFile;
    }

    //const EdaDataFile& FileModel::GetStepEdaDataFile(std::string stepName) const
	//{		
	//	auto findIt = m_stepsByName.find(stepName);
	//	if (findIt != m_stepsByName.end())
	//	{
	//		return findIt->second->GetEdaDataFile();
	//	}
	//	else
	//	{
	//		return EdaDataFile::EMPTY;
	//	}		
	//}

	//const EdaDataFile& FileModel::GetFirstStepEdaDataFile() const
	//{
	//	if (!m_stepsByName.empty())
	//	{
	//		return m_stepsByName.begin()->second->GetEdaDataFile();
	//	}
	//	else
	//	{
	//		return EdaDataFile::EMPTY;
	//	}
	//}
}