#include "FileArchive.h"
#include <filesystem>

namespace Odb::Lib::FileModel::Design
{

	FileArchive::FileArchive(std::string directoryPath)
		: m_path(directoryPath)
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
		std::filesystem::path designPath(m_path);

		if (!std::filesystem::exists(designPath)) return false;

		if (std::filesystem::is_regular_file(designPath))
		{
			if (designPath.extension() == ".tar.gz")
			{
				// unzip and change designPath to point to the unzipped directory

			}
		}

		if (std::filesystem::is_directory(designPath))
		{
			return ParseDesignDirectory(designPath);
		}

		return true;
	}

	bool FileArchive::ParseDesignDirectory(std::filesystem::path path)
	{
		std::filesystem::path designPath(path);

		if (!std::filesystem::exists(designPath)) return false;
		else if (!std::filesystem::is_directory(designPath)) return false;

		m_productName = designPath.filename().string();

		auto stepsPath = designPath / "steps";
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

		return true;
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