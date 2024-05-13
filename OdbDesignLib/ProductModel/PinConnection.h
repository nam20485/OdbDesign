#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Component.h"
#include "Pin.h"
#include "../ProtoBuf/pinconnection.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT PinConnection : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::PinConnection>
	{
	public:
		PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin);		
		PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin, const std::string& name);
		//~PinConnection();	

		std::shared_ptr<Pin> GetPin() const;
		std::shared_ptr<Component> GetComponent() const;

		static std::string MakeName(const Component& component, const Pin& pin);

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::PinConnection> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::PinConnection& message) override;

		typedef std::vector<std::shared_ptr<PinConnection>> Vector;
		typedef std::map<std::string, std::shared_ptr<PinConnection>> StringMap;

	private:
		std::string m_name;
		std::shared_ptr<Component> m_pComponent;
		std::shared_ptr<Pin> m_pPin;

	};
}
