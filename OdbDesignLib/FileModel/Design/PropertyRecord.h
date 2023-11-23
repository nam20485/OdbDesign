#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../odbdesign_export.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/common.pb.h"

namespace Odb::Lib::FileModel::Design
{
	struct ODBDESIGN_EXPORT PropertyRecord : public IProtoBuffable<Odb::Lib::Protobuf::PropertyRecord>
	{
		// data members
		std::string name;
		std::string value;
		std::vector<float> floatValues;

		// constants
		constexpr inline static const char* RECORD_TOKEN = "PRP";

		// typedefs
		typedef std::map<std::string, std::shared_ptr<PropertyRecord>> StringMap;
		typedef std::vector<std::shared_ptr<PropertyRecord>> Vector;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::PropertyRecord> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::PropertyRecord& message) override;
	};
}
