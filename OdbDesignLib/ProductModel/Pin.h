#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../ProtoBuf/pin.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Pin : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Pin>
	{
	public:
		Pin(const std::string& name, unsigned int index);
		//~Pin();

		std::string GetName() const;
		unsigned int GetIndex() const;

		typedef std::vector<std::shared_ptr<Pin>> Vector;
		typedef std::map<std::string, std::shared_ptr<Pin>> StringMap;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Pin> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Pin& message) override;

	private:
		std::string m_name;
		unsigned int m_index;		

	};
} // namespace Odb::Lib::ProductModel
