#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Package.h"
#include "../enums.h"
#include "Part.h"
#include "../ProtoBuf/component.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Component : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Component>
	{
	public:
		Component();
		Component(const std::string& refDes, const std::string& partName, std::shared_ptr<Package> pPackage, unsigned int index, BoardSide side, std::shared_ptr<Part> pPart);
		~Component();

		std::string GetRefDes() const;
		std::string GetPartName() const;
		std::shared_ptr<Package> GetPackage() const;
		unsigned int GetIndex() const;
		BoardSide GetSide() const;
		std::shared_ptr<Part> GetPart() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Component> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Component& message) override;

		//static Component* MakeEmpty();

		typedef std::vector<std::shared_ptr<Component>> Vector;
		typedef std::map<std::string, std::shared_ptr<Component>> StringMap;

	private:
		std::string m_refDes;
		std::string m_partName;		
		std::shared_ptr<Package> m_pPackage;
		unsigned int m_index;
		BoardSide m_side;
		std::shared_ptr<Part> m_pPart;
	};
}
