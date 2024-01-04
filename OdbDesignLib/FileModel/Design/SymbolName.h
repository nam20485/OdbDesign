#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include "../../enums.h"
#include "../../odbdesign_export.h"
#include "../parse_error.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/symbolname.pb.h"

// TODO: add SymbolName serialization

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT SymbolName : public IProtoBuffable<Odb::Lib::Protobuf::SymbolName>
	{
	public:
		SymbolName();		

		std::string GetName() const;
		UnitType GetUnitType() const;

		bool Parse(const std::filesystem::path& path, const std::string& line, int lineNumber);

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::SymbolName> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::SymbolName& message) override;

		typedef std::vector<std::shared_ptr<SymbolName>> Vector;
		typedef std::map<std::string, std::shared_ptr<SymbolName>> StringMap;

	private:
		std::string m_name;
		UnitType m_unitType;		

	};
}
