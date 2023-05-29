#include "Design.h"
#include "Package.h"

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
		if (m_pFileModel == nullptr) return false;

		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;

		auto& pStepDirectory = steps.begin()->second;
		const auto& edaData = pStepDirectory->GetEdaDataFile();
		const auto& netRecords = edaData.GetNetRecords();

		for (const auto& pNetRecord : netRecords)
		{
			auto pNet = std::make_shared<Net>(pNetRecord->name);
			m_netsByName[pNet->GetName()] = pNet;
		}

		return true;
	}

	bool Design::BuildPackages()
	{
		if (m_pFileModel == nullptr) return false;

		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;

		auto& pStepDirectory = steps.begin()->second;
		const auto& edaData = pStepDirectory->GetEdaDataFile();
		const auto& packageRecords = edaData.GetPackageRecords();

		for (const auto& pPackageRecord : packageRecords)
		{
			auto pPackage = std::make_shared<Package>(pPackageRecord->name);

			for (const auto& pPinRecord : pPackageRecord->m_pinRecords)
			{				
				// TODO: figure out how to handle size_t -> non-size_t conversion
				pPackage->AddPin(pPinRecord->name, static_cast<unsigned long>(pPinRecord->index));
			}

			m_packagesByName[pPackage->GetName()] = pPackage;
		}		

		return true;
	}

} // namespace Odb::Lib::ProductModel