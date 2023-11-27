#include "Design.h"
#include "Design.h"
#include "Package.h"
#include "Logger.h"

namespace Odb::Lib::ProductModel
{	
	Design::Design()
	{
	}

	Design::~Design()
	{
	}

	bool Design::Build(std::string path)
	{
		auto pFileModel = std::make_shared<FileModel::Design::FileArchive>(path);
		if (pFileModel->ParseFileModel())
		{
			return Build(pFileModel);
		}
		return false;
	}

	bool Design::Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel)
	{		
		if (pFileModel == nullptr) return false;

		m_pFileModel = pFileModel;

		// atomic elements
		if (! BuildNets()) return false;
		if (! BuildPackages()) return false;
		if (! BuildAllParts()) return false;
		if (! BuildAllComponents()) return false;

		// built from relationships between atomic elements
		if (! BuildPlacementsFromComponentsFiles()) return false;		

		return true;
	}

	std::shared_ptr<FileModel::Design::FileArchive> Design::GetFileModel() const
	{
		return m_pFileModel;
	}

	bool Design::BuildAllComponents()
	{
		if (m_pFileModel == nullptr) return false;
		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;
		auto& pStepDirectory = steps.begin()->second;

		// top components layer
		auto pTopComponentsFile = pStepDirectory->GetTopComponentsFile();
		if (pTopComponentsFile == nullptr) return false;
		if (! BuildComponents(pTopComponentsFile)) return false;

		// bottom layer components
		auto pBottomComponentsFile = pStepDirectory->GetBottomComponentsFile();
		if (pBottomComponentsFile == nullptr) return false;
		if (!BuildComponents(pBottomComponentsFile)) return false;

		return true;
	}

	bool Design::BuildComponents(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile)
	{
		const auto& componentRecords = pComponentsFile->GetComponentRecords();
		for (const auto& pComponentRecord : componentRecords)
		{
			auto& pPackage = m_packages[pComponentRecord->pkgRef];
			auto index = pComponentRecord->index;
			auto& pPart = m_partsByName[pComponentRecord->partName];
			auto pComponent = std::make_shared<Component>(pComponentRecord->compName, pComponentRecord->partName, pPackage, index, pComponentsFile->GetSide(), pPart);

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
			auto pNet = std::make_shared<Net>(pNetRecord->name, pNetRecord->index);
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
			auto index = pPackageRecord->index;
			auto pPackage = std::make_shared<Package>(pPackageRecord->name, index);

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

	bool Design::BuildAllParts()
	{
		if (m_pFileModel == nullptr) return false;

		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;

		auto& pStepDirectory = steps.begin()->second;

		auto pTopComponentsFile = pStepDirectory->GetTopComponentsFile();
		if (pTopComponentsFile == nullptr) return false;
		if (! BuildParts(pTopComponentsFile)) return false;

		auto pBottomComponentsFile = pStepDirectory->GetBottomComponentsFile();
		if (pBottomComponentsFile == nullptr) return false;
		if (! BuildParts(pBottomComponentsFile)) return false;

		return true;
	}

	bool Design::BuildParts(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile)
	{
		const auto& componentRecords = pComponentsFile->GetComponentRecords();
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

	bool Design::BuildPlacementsFromComponentsFiles()
	{
		if (m_pFileModel == nullptr) return false;

		const auto& steps = m_pFileModel->GetStepsByName();
		if (steps.empty()) return false;

		// "first" step when there are >1 steps
		auto& pStepDirectory = steps.begin()->second;

		auto pTopComponentsFile = pStepDirectory->GetTopComponentsFile();
		if (pTopComponentsFile == nullptr) return false;
		if (! BuildPlacementsFromComponentsFile(pTopComponentsFile)) return false;

		auto pBottomComponentsFile = pStepDirectory->GetBottomComponentsFile();
		if (pBottomComponentsFile == nullptr) return false;
		if (!BuildPlacementsFromComponentsFile(pBottomComponentsFile)) return false;

		return true;
	}

	bool Design::BuildPlacementsFromComponentsFile(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile)
	{
		const auto& componentRecords = pComponentsFile->GetComponentRecords();
		for (const auto& pComponentRecord : componentRecords)
		{
			const auto& toeprintRecords = pComponentRecord->m_toeprintRecords;
			for (const auto& pToeprintRecord : toeprintRecords)
			{
				auto& toeprintName = pToeprintRecord->name;
				auto pinNumber = pToeprintRecord->pinNumber;
				auto netNumber = pToeprintRecord->netNumber;
				//auto subnetNumber = pToeprintRecord->subnetNumber;

				if (netNumber < m_nets.size())
				{
					auto& pComponent = m_componentsByName[pComponentRecord->compName];
					if (pComponent == nullptr) return false;

					auto pPin = pComponent->GetPackage()->GetPin(pinNumber);
					if (pPin == nullptr) return false;

					auto& pNet = m_nets[netNumber];
					if (pNet == nullptr) return false;

					if (!pNet->AddPinConnection(pComponent, pPin, toeprintName)) return false;
				}
				else
				{
					logerror("netNumber out of range: " + std::to_string(netNumber) + ", size = " + std::to_string(m_nets.size()));
				}
			}
		}

		return true;
	}

	bool Design::BuildPlacementsFromEdaDataFile()
	{
		return false;
	}

} // namespace Odb::Lib::ProductModel