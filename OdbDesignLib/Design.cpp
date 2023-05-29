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

		if (! BuildNets()) return false;
		if (! BuildPackages()) return false;
		if (! BuildComponents()) return false;
		if (! BuildParts()) return false;
		if (! BuildPlacements()) return false;

		return true;
	}

	bool Design::BuildComponents()
	{
		if (m_pFileModel == nullptr) return false;
		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;
		auto& pStepDirectory = steps.begin()->second;

		// top components layer
		auto pTopComponentsLayerDir = pStepDirectory->GetTopComponentLayerDir();
		if (pTopComponentsLayerDir != nullptr) return false;
		if (! BuildLayerComponents(pTopComponentsLayerDir)) return false;

		// bottom layer components
		auto pBottomComponentsLayerDir = pStepDirectory->GetBottomComponentLayerDir();
		if (pBottomComponentsLayerDir == nullptr) return false;
		if (!BuildLayerComponents(pBottomComponentsLayerDir)) return false;

		return true;
	}

	bool Design::BuildLayerComponents(std::shared_ptr<Odb::Lib::FileModel::Design::ComponentLayerDirectory>& pComponentsLayerDir)
	{
		const auto& componentRecords = pComponentsLayerDir->GetComponentRecords();
		for (const auto& pComponentRecord : componentRecords)
		{
			auto& pPackage = m_packages[pComponentRecord->pkgRef];
			auto index = static_cast<unsigned int>(m_components.size());
			auto pComponent = std::make_shared<Component>(pComponentRecord->compName, pComponentRecord->partName, pPackage, index);

			m_components.push_back(pComponent);
			m_componentsByName[pComponent->GetRefDes()] = pComponent;
		}

		return true;
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
			auto pNet = std::make_shared<Net>(pNetRecord->name, static_cast<unsigned int>(m_nets.size()));
			m_nets.push_back(pNet);
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
			auto pPackage = std::make_shared<Package>(pPackageRecord->name, static_cast<unsigned int>(m_packages.size()));

			for (const auto& pPinRecord : pPackageRecord->m_pinRecords)
			{				
				// TODO: figure out how to handle size_t -> non-size_t conversion
				pPackage->AddPin(pPinRecord->name);
			}

			m_packages.push_back(pPackage);
			m_packagesByName[pPackage->GetName()] = pPackage;
		}		

		return true;
	}

	bool Design::BuildParts()
	{
		if (m_pFileModel == nullptr) return false;
		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;
		auto& pStepDirectory = steps.begin()->second;

		auto pTopComponentsLayerDir = pStepDirectory->GetTopComponentLayerDir();
		if (pTopComponentsLayerDir == nullptr) return false;

		const auto& componentRecords = pTopComponentsLayerDir->GetComponentRecords();
		for (const auto& pComponentRecord : componentRecords)
		{
			auto& partName = pComponentRecord->partName;
			auto findIt = m_partsByName.find(partName);
			if (findIt == m_partsByName.end())
			{
				auto pPart = std::make_shared<Part>(partName);				
				m_partsByName[partName] = pPart;
			}
		}

		return true;		
	}

	bool Design::BuildPlacements()
	{
		if (m_pFileModel == nullptr) return false;
		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;
		auto& pStepDirectory = steps.begin()->second;

		auto pTopComponentsLayerDir = pStepDirectory->GetTopComponentLayerDir();
		if (pTopComponentsLayerDir == nullptr) return false;

		const auto& componentRecords = pTopComponentsLayerDir->GetComponentRecords();
		for (const auto& pComponentRecord : componentRecords)
		{
			auto pPackage = m_packages[pComponentRecord->pkgRef];
			auto index = static_cast<unsigned int>(m_components.size());
			auto pComponent = std::make_shared<Component>(pComponentRecord->compName, pComponentRecord->partName, pPackage, index);

			m_components.push_back(pComponent);
			m_componentsByName[pComponent->GetRefDes()] = pComponent;
		}

		return true;
	}

} // namespace Odb::Lib::ProductModel