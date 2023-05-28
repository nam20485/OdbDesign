#include "Design.h"

namespace Odb::Lib::ProductModel
{	
	Design::Design()
	{
	}

	Design::~Design()
	{
	}

	bool Design::Build(std::string designDirectory)
	{
		auto pFileModel = std::make_shared<FileModel::Design::FileModel>(designDirectory);
		if (pFileModel->ParseFileModel())
		{
			return Build(pFileModel);
		}
		return false;
	}

	bool Design::Build(std::shared_ptr<FileModel::Design::FileModel> pFileModel)
	{
		m_pFileModel = pFileModel;
		return Build();		
	}

	bool Design::Build()
	{
		if (m_pFileModel == nullptr) return false;

		BuildNets();

		return true;
	}

	bool Design::BuildComponents()
	{
		return false;
	}

	bool Design::BuildNets()
	{		
		const auto& steps = m_pFileModel->GetStepsByName();

		if (steps.empty()) return false;

		auto& pStepDirectory = steps.begin()->second;
		const auto& edaData = pStepDirectory->GetEdaDataFile();
		const auto& netRecords = edaData.GetNetRecords();

		for (const auto& netRecord : netRecords)
		{
			auto pNet = std::make_shared<Net>(netRecord->name);
			m_netsByName[pNet->GetName()] = pNet;
		}

		return true;
	}

	bool Design::BuildPackages()
	{
		return false;
	}

} // namespace Odb::Lib::ProductModel