#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <crow.h>
#include "IJsonable.h"
#include "odbdesign_export.h"


namespace Odb::Lib
{
	template<typename TPbMessage>
	class IProtoBuffable : public Utils::IJsonable
	{
	public:		
		virtual std::unique_ptr<TPbMessage> to_protobuf() const = 0;
		virtual void from_protobuf(const TPbMessage& message) = 0;

		// Inherited via IJsonConvertable
		std::string to_json() const override;
		void from_json(const std::string& json) override;		
				
	protected:
		// abstract class
		IProtoBuffable()		
		{}

		// TMessage MUST derive from Message (must use this until template type contraints support is added)
		static_assert(std::is_base_of<google::protobuf::Message, TPbMessage>::value, "template parameter type TPbMessage must derive from Protobuf Message class");
		
	};

	template<typename TPbMessage>
	std::string IProtoBuffable<TPbMessage>::to_json() const
	{	
		// use default options
		google::protobuf::util::JsonOptions jsonOptions;

		std::string json;
		auto status = google::protobuf::util::MessageToJsonString(*to_protobuf(), &json, jsonOptions);
		if (!status.ok()) json.clear();
		return json;
	}

	template<typename TPbMessage>
	inline void IProtoBuffable<TPbMessage>::from_json(const std::string& json)
	{			
		google::protobuf::StringPiece sp_json(json);
		// use default options
		google::protobuf::util::JsonOptions jsonOptions;

		auto pMessage = std::unique_ptr<TPbMessage>();
		auto status = google::protobuf::util::JsonStringToMessage(sp_json, pMessage.get());
		if (!status.ok()) return;		
		from_protobuf(*pMessage);		
	}	
}
