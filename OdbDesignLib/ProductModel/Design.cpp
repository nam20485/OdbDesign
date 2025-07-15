#include "Design.h"
#include "Design.h"
#include "Package.h"
#include "Logger.h"
#include "../enums.h"
#include "Part.h"
#include <memory>
#include "Net.h"
#include "../FileModel/Design/FileArchive.h"
#include "../FileModel/Design/EdaDataFile.h"


namespace Odb::Lib::ProductModel
{	
	Design::Design()
	{
	}

	Design::~Design()
	{
		m_components.clear();
		m_componentsByName.clear();
		m_nets.clear();
		m_netsByName.clear();
		m_packages.clear();
		m_packagesByName.clear();
		m_parts.clear();
		m_partsByName.clear();
	}

	const std::string& Design::GetName() const
	{
		return m_name;
	}

	const std::string& Design::GetProductModel() const
	{
		return m_productModel;
	}

	const Net::Vector& Design::GetNets() const
	{
		return m_nets;
	}

	const Net::StringMap Design::GetNetsByName() const
	{
		return m_netsByName;
	}

	const Package::Vector& Design::GetPackages() const
	{
		return m_packages;
	}

	const Package::StringMap& Design::GetPackagesByName() const
	{
		return m_packagesByName;
	}

	const Component::Vector& Design::GetComponents() const
	{
		return m_components;
	}

	const Component::StringMap& Design::GetComponentsByName() const
	{
		return m_componentsByName;
	}

	std::shared_ptr<Component> Design::GetComponent(const std::string& refDes) const
	{
		auto findIt = m_componentsByName.find(refDes);
		if (findIt != m_componentsByName.end())
		{
			return findIt->second;
		}

		return nullptr;
	}

	const Part::Vector& Design::GetParts() const
	{
		return m_parts;
	}

	const Part::StringMap& Design::GetPartsByName() const
	{
		return m_partsByName;
	}

	std::shared_ptr<Net> Design::GetNet(const std::string& name) const
	{
		auto findIt = m_netsByName.find(name);
		if (findIt != m_netsByName.end())
		{
			return findIt->second;
		}
		return nullptr;
	}

	std::shared_ptr<Net> Design::GetNoneNet() const
	{
		return GetNet(NONE_NET_NAME);
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

		//m_productModel = m_pFileModel->GetProductName();
		//m_name = m_pFileModel->GetProductName();

		// atomic elements
		if (! BuildNets()) return false;
		if (! BuildPackages()) return false;
		if (! BuildAllParts()) return false;
		if (! BuildAllComponents()) return false;

		// built from relationships between atomic elements
		if (! BuildPlacementsFromComponentsFiles()) return false;

		// already built from BuildPlacements()
		//if (! BuildNoneNet()) return false;
		//if (! BreakSinglePinNets()) return false;

		if (CLIP_FILEMODEL_AFTER_BUILD)
		{
			ClipFileModel();
		}

		return true;
	}

	std::shared_ptr<FileModel::Design::FileArchive> Design::GetFileModel() const
	{
		return m_pFileModel;
	}

	void Design::ClipFileModel()
	{
		m_pFileModel = nullptr;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Design> Design::to_protobuf() const
	{
		auto pDesignMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::Design>();
		pDesignMsg->set_name(m_name);
		pDesignMsg->set_productmodel(m_productModel);

		if (m_pFileModel != nullptr)
		{
			pDesignMsg->mutable_filemodel()->CopyFrom(*m_pFileModel->to_protobuf());
		}

		for (const auto& pNet : m_nets)
		{
			pDesignMsg->add_nets()->CopyFrom(*pNet->to_protobuf());
		}

		for (const auto& kvNet : m_netsByName)
		{
			(*pDesignMsg->mutable_netsbyname())[kvNet.first] = *kvNet.second->to_protobuf();
		}

		for (const auto& pPackage : m_packages)
		{
			pDesignMsg->add_packages()->CopyFrom(*pPackage->to_protobuf());
		}

		for (const auto& kvPackage : m_packagesByName)
		{
			(*pDesignMsg->mutable_packagesbyname())[kvPackage.first] = *kvPackage.second->to_protobuf();
		}

		for (const auto& pComponent : m_components)
		{
			pDesignMsg->add_components()->CopyFrom(*pComponent->to_protobuf());
		}

		for (const auto& kvComponent : m_componentsByName)
		{
			(*pDesignMsg->mutable_componentsbyname())[kvComponent.first] = *kvComponent.second->to_protobuf();
		}

		for (const auto& pPart : m_parts)
		{
			pDesignMsg->add_parts()->CopyFrom(*pPart->to_protobuf());
		}

		for (const auto& kvPart : m_partsByName)
		{
			(*pDesignMsg->mutable_partsbyname())[kvPart.first] = *kvPart.second->to_protobuf();
		}		

		return pDesignMsg;
	}

	void Design::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Design& message)
	{
		m_name = message.name();
		m_productModel = message.productmodel();

		m_pFileModel = std::make_shared<FileModel::Design::FileArchive>();
		m_pFileModel->from_protobuf(message.filemodel());

		for (const auto& pNetMsg : message.nets())
		{
			auto pNet = std::make_shared<Net>("", -1);
			pNet->from_protobuf(pNetMsg);
			m_nets.push_back(pNet);
		}

		for (const auto& kvNetMsg : message.netsbyname())
		{
			auto pNet = std::make_shared<Net>("", -1);
			pNet->from_protobuf(kvNetMsg.second);
			m_netsByName[kvNetMsg.first] = pNet;
		}

		for (const auto& pPackageMsg : message.packages())
		{
			auto pPackage = std::make_shared<Package>("", -1);
			pPackage->from_protobuf(pPackageMsg);
			m_packages.push_back(pPackage);
		}

		for (const auto& kvPackageMsg : message.packagesbyname())
		{
			auto pPackage = std::make_shared<Package>("", -1);
			pPackage->from_protobuf(kvPackageMsg.second);
			m_packagesByName[kvPackageMsg.first] = pPackage;
		}

		for (const auto& pComponentMsg : message.components())
		{
			auto pComponent = std::make_shared<Component>("", "", nullptr, -1, BoardSide::BsNone, nullptr);
			pComponent->from_protobuf(pComponentMsg);
			m_components.push_back(pComponent);
		}

		for (const auto& kvComponentMsg : message.componentsbyname())
		{
			auto pComponent = std::make_shared<Component>("", "", nullptr, -1, BoardSide::BsNone, nullptr);
			pComponent->from_protobuf(kvComponentMsg.second);
			m_componentsByName[kvComponentMsg.first] = pComponent;
		}

		for (const auto& pPartMsg : message.parts())
		{
			auto pPart = std::make_shared<Part>("");
			pPart->from_protobuf(pPartMsg);
			m_parts.push_back(pPart);
		}

		for (const auto& kvPartMsg : message.partsbyname())
		{
			auto pPart = std::make_shared<Part>("");
			pPart->from_protobuf(kvPartMsg.second);
			m_partsByName[kvPartMsg.first] = pPart;
		}
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

	bool Design::BuildVias()
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
				m_parts.push_back(pPart);
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
				auto& refDes = pComponentRecord->compName;
				//auto subnetNumber = pToeprintRecord->subnetNumber;

				// -1 means no connection for the component pin
				if (netNumber != static_cast<unsigned int>(-1))
				{
					if (!CreatePinConnection(refDes, netNumber, pinNumber, toeprintName)) return false;
				}
			}
		}

		return true;
	}

	bool Design::CreatePinConnection(const std::string& refDes, unsigned int netNumber, unsigned int pinNumber, const std::string& pinName)
	{
		if (netNumber < m_nets.size())
		{
			auto& pComponent = m_componentsByName[refDes];
			if (pComponent == nullptr) return false;

			auto pPin = pComponent->GetPackage()->GetPin(pinNumber);
			if (pPin == nullptr) return false;

			auto& pNet = m_nets[netNumber];
			if (pNet == nullptr) return false;

			if (!pNet->AddPinConnection(pComponent, pPin)) return false;
			return true;
		}
		else
		{
			logwarn("netNumber out of range: " + std::to_string(netNumber) + ", size = " + std::to_string(m_nets.size()));
		}	

		return false;
	}

	bool Design::BuildNoneNet()
	{
		auto pStepDirectory = m_pFileModel->GetStepDirectory();
		if (pStepDirectory == nullptr) return false;		

		const auto& edaData = pStepDirectory->GetEdaDataFile();
		auto findIt = edaData.GetNetRecordsByName().find(NONE_NET_NAME);
		if (findIt == edaData.GetNetRecordsByName().end()) return false;
		
		auto& pNoneNet = findIt->second;
		if (pNoneNet == nullptr) return false;

		if (!CreateNetConnections(pNoneNet, pStepDirectory)) return false;

		return true;
	}

	bool Design::BreakSinglePinNets()
	{
		const auto& pNoneNet = GetNoneNet();
		if (pNoneNet == nullptr) return false;

		auto& pinConnections = pNoneNet->GetPinConnections();		
		while (!pinConnections.empty())
		{
			const auto& pExistingPinConnection = pinConnections.back();				
			
			std::string newNetName = "$NC_" + pExistingPinConnection->GetComponent()->GetRefDes() + "_" + pExistingPinConnection->GetPin()->GetName();
			auto pNewNet = std::make_shared<Net>(newNetName, -1);
			pNewNet->AddPinConnection(pExistingPinConnection->GetComponent(), pExistingPinConnection->GetPin());
			m_nets.push_back(pNewNet);
			m_netsByName[pNewNet->GetName()] = pNewNet;

			pinConnections.pop_back();						
		}			

		return true;
	}

	bool Design::BuildPlacementsFromEdaDataFile()
	{
		auto pStepDirectory = m_pFileModel->GetStepDirectory();
		if (pStepDirectory == nullptr) return false;

		const auto& edaData = pStepDirectory->GetEdaDataFile();
		
		for (const auto& pNetRecord : edaData.GetNetRecords())
		{			
			if (!CreateNetConnections(pNetRecord, pStepDirectory)) return false;			
		}		

		return true;
	}

	bool Design::CreateNetConnections(const std::shared_ptr<Odb::Lib::FileModel::Design::EdaDataFile::NetRecord>& pNetRecord, const std::shared_ptr<FileModel::Design::StepDirectory>& pStepDirectory)
	{
		for (const auto& pSubnetRecord : pNetRecord->m_subnetRecords)
		{
			if (pSubnetRecord->type == FileModel::Design::EdaDataFile::NetRecord::SubnetRecord::Type::Toeprint)
			{										
				const FileModel::Design::ComponentsFile* pComponentsFileToUse = nullptr;

				auto side = pSubnetRecord->side;
				if (side == BoardSide::Top)
				{
					pComponentsFileToUse = pStepDirectory->GetTopComponentsFile();
				}
				else //if (side == BoardSide::Bottom)
				{
					pComponentsFileToUse = pStepDirectory->GetBottomComponentsFile();
				}

				if (pComponentsFileToUse == nullptr) return false;

				auto componentNumber = pSubnetRecord->componentNumber;				
				if (componentNumber >= pComponentsFileToUse->GetComponentRecords().size()) return false;

				auto& pComponentRecord = pComponentsFileToUse->GetComponentRecords()[componentNumber];
				if (pComponentRecord == nullptr) return false;

				auto toeprintNumber = pSubnetRecord->toeprintNumber;
				if (toeprintNumber >= pComponentRecord->m_toeprintRecords.size()) return false;
				
				auto& pToeprintRecord = pComponentRecord->m_toeprintRecords[toeprintNumber];
				if (pToeprintRecord == nullptr) return false;

				auto& toeprintName = pToeprintRecord->name;
				auto pinNumber = pToeprintRecord->pinNumber;
				auto netNumber = pToeprintRecord->netNumber;
				auto& refDes = pComponentRecord->compName;
				//auto subnetNumber = pToeprintRecord->subnetNumber;

				if (!CreatePinConnection(refDes, netNumber, pinNumber, toeprintName)) return false;			
			}
			else if (pSubnetRecord->type == FileModel::Design::EdaDataFile::NetRecord::SubnetRecord::Type::Via)
			{
				// ?
			}
		}

		return true;
	}

} // namespace Odb::Lib::ProductModel