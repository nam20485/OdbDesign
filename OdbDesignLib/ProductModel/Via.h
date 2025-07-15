#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../enums.h"
#include "../ProtoBuf/via.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Via : public IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Via>
	{
	public:
		Via();

		std::string GetName() const;
		BoardSide GetSide() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Via> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Via& message) override;

		typedef std::vector<std::shared_ptr<Via>> Vector;
		typedef std::map<std::string, std::shared_ptr<Via>> StringMap;

	private:
		std::string m_name;
		BoardSide m_side;

	};
} // namespace Odb::Lib::ProductModel
