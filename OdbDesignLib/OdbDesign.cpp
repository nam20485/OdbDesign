#include "OdbDesign.h"
#include <filesystem>

namespace OdbDesign::Lib
{

	OdbDesign::OdbDesign(std::string directoryPath)
		: m_path(directoryPath)
	{
	}

	OdbDesign::~OdbDesign()
	{
	}

	std::string OdbDesign::GetPath() const
	{
		return m_path;
	}

	std::string OdbDesign::GetProductName() const
	{
		return m_productName;
	}

	const Step::StringMap& OdbDesign::GetStepsByName() const { return m_stepsByName; }

	bool OdbDesign::ParseDesign()
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

	bool OdbDesign::ParseDesignDirectory(std::filesystem::path path)
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
				auto pStep = std::make_shared<Step>(d.path());
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