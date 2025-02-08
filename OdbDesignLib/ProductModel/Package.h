#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Pin.h"
#include "../ProtoBuf/package.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Package : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Package>
	{
	public:
		Package(const std::string& name, unsigned int index);
		~Package();

		std::string GetName() const;		
		unsigned int GetIndex() const;

		void AddPin(const std::string& name);
		std::shared_ptr<Pin> GetPin(const std::string& name) const;
		std::shared_ptr<Pin> GetPin(unsigned int index) const;
		const Pin::StringMap& GetPinsByName() const;		
		const Pin::Vector& GetPins() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Package> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Package& message) override;

		typedef std::vector<std::shared_ptr<Package>> Vector;
		typedef std::map<std::string, std::shared_ptr<Package>> StringMap;

	private:
		std::string m_name;
		Pin::Vector m_pins;		
		Pin::StringMap m_pinsByName;
		unsigned int m_index;

	};
} // namespace Odb::Lib::ProductModel
