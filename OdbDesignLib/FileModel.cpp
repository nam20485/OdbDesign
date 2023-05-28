#include "FileModel.h"
#include <filesystem>

namespace OdbDesign::Lib::FileModel
{

	FileModel::FileModel(std::string directoryPath)
		: m_path(directoryPath)
	{
	}

	FileModel::~FileModel()
	{
	}

	std::string FileModel::GetPath() const
	{
		return m_path;
	}

	std::string FileModel::GetProductName() const
	{
		return m_productName;
	}

	const StepDirectory::StringMap& FileModel::GetStepsByName() const { return m_stepsByName; }

	bool FileModel::ParseFileModel()
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

	bool FileModel::ParseDesignDirectory(std::filesystem::path path)
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
}