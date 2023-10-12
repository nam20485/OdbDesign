#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <crow.h>
#include "IJsonable.h"
#include "odbdesign_export.h"

using namespace google::protobuf;
using namespace google::protobuf::util;


namespace Odb::Lib
{
	template<typename TPbMessage>
	class IProtoBuffable : public Utils::IJsonable
	{
	public:		
		virtual std::unique_ptr<TPbMessage> to_protobuf() const = 0;
		virtual void from_protobuf(const TPbMessage& message) = 0;

		// Inherited via IJsonConvertable
		std::string to_json() const;		
		void from_json(const std::string& json) override;		
				
	protected:
		// abstract class
		IProtoBuffable()		
		{}

		// TMessage MUST derive from Message (must use this until template type contraints support is added)
		static_assert(std::is_base_of<Message, TPbMessage>::value, "template parameter type TPbMessage must derive from Protobuf Message class");
		
	};

	template<typename TPbMessage>
	std::string IProtoBuffable<TPbMessage>::to_json() const
	{	
		// use default options
		JsonOptions jsonOptions;	

		std::string json;
		auto status = MessageToJsonString(*to_protobuf(), &json, jsonOptions);
		if (!status.ok()) json.clear();
		return json;
	}

	template<typename TPbMessage>
	inline void IProtoBuffable<TPbMessage>::from_json(const std::string& json)
	{			
		StringPiece sp_json(json);
		// use default options
		JsonOptions jsonOptions;

		auto pMessage = std::unique_ptr<TPbMessage>();
		auto status = JsonStringToMessage(sp_json, pMessage.get());
		if (!status.ok()) return;		
		from_protobuf(*pMessage);		
	}	
}
