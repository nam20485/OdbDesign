#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "PinConnection.h"
#include "Component.h"
#include "Pin.h"
#include "../ProtoBuf/net.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Net : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Net>
	{
	public:
		Net(const std::string& name, unsigned int index);
		~Net();

		std::string GetName() const;
		PinConnection::Vector& GetPinConnections();
		unsigned int GetIndex() const;
		bool AddPinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin);

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Net> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Net& message) override;

		typedef std::vector<std::shared_ptr<Net>> Vector;
		typedef std::map<std::string, std::shared_ptr<Net>> StringMap;

	private:
		std::string m_name;
		PinConnection::Vector m_pinConnections;
		unsigned int m_index;

	};
}
