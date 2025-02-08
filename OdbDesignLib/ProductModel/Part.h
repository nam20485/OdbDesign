#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../ProtoBuf/part.pb.h"
#include "../IProtoBuffable.h"	

namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Part : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Part>
	{
	public:
		Part(const std::string& name);

		std::string GetName() const;

		typedef std::vector<std::shared_ptr<Part>> Vector;
		typedef std::map<std::string, std::shared_ptr<Part>> StringMap;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Part> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Part& message) override;

	private:
		std::string m_name;

	};
}
