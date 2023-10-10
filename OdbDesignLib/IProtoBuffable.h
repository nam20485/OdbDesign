#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <crow.h>
#include "IJsonConvertable.h"
#include "export.h"

using namespace google::protobuf;
using namespace google::protobuf::util;


namespace Odb::Lib
{
	template<typename TPbMessage>
	class IProtoBuffable : public IJsonConvertable, public crow::returnable
	{
	public:		
		virtual std::unique_ptr<TPbMessage> to_protobuf() const = 0;
		virtual void from_protobuf(const TPbMessage& message) = 0;

		// Inherited via IJsonConvertable
		std::string to_json() const;		
		void from_json(const std::string& json) override;

		// Inherited via returnable
		std::string dump() const override;
				
	protected:
		// abstract class
		IProtoBuffable()
			: returnable("application/json")
		{}

		// TMessage MUST derive from Message (must use this until template type contraints are added)
		static_assert(std::is_base_of<Message, TPbMessage>::value, "type parameter TMessage must derive from Message class");
		
	};

	template<typename TPbMessage>
	std::string IProtoBuffable<TPbMessage>::to_json() const
	{
		std::string json;

		// use default options
		google::protobuf::util::JsonOptions jsonOptions;		
		auto status = MessageToJsonString(*to_protobuf(), &json, jsonOptions);
		if (!status.ok()) json.clear();

		return json;
	}

	template<typename TPbMessage>
	inline void IProtoBuffable<TPbMessage>::from_json(const std::string& json)
	{
		auto pMessage = std::unique_ptr<TPbMessage>();
		//auto pMessage = new TPbMessage();
		google::protobuf::StringPiece sp_json(json);
		google::protobuf::util::JsonOptions jsonOptions;

		auto status = google::protobuf::util::JsonStringToMessage(sp_json, pMessage.get());
		if (!status.ok()) return;		
		from_protobuf(*pMessage);		
	}

	template<typename TPbMessage>
	inline std::string IProtoBuffable<TPbMessage>::dump() const
	{
		return to_json();
	}	
}
