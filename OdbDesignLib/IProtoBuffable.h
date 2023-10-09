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
	template<typename TMessage>
	class IProtoBuffable : public IJsonConvertable, public crow::returnable
	{
	public:		
		virtual TMessage* to_protobuf() const = 0;
		virtual void from_protobuf(const TMessage& message) = 0;

		std::string to_json() const;
		//void from_json(const std::string& json);
		//static TMessage* message_from_json(const std::string& json);
	
		// TMessage MUST derive from Message (must use this until template type contraints are added)
		static_assert(std::is_base_of<Message, TMessage>::value, "type parameter TMessage must derive from Message class");	

	protected:
		// abstract class
		IProtoBuffable()
			: returnable("application/json")
		{
		}

		// Inherited via returnable
		std::string dump() const override
		{
			return to_json();
		}

	};

	template<typename TMessage>
	std::string IProtoBuffable<TMessage>::to_json() const
	{
		std::string json;

		// use default options
		google::protobuf::util::JsonOptions jsonOptions;		
		auto pMessage = static_cast<Message*>(to_protobuf());
		auto status = MessageToJsonString(*pMessage, &json, jsonOptions);
		if (!status.ok()) json.clear();

		return json;
	}

	//template<typename TMessage>
	//TMessage* IProtoBuffable<TMessage>::message_from_json(const std::string& json)
	//{
	//	auto pMessage = new TMessage();
	//	//Message* pMessage = nullptr;

	//	google::protobuf::StringPiece sp(json);
	//	google::protobuf::util::JsonOptions jsonOptions;
	//	auto status = google::protobuf::util::JsonStringToMessage(sp, pMessage);

	//	return pMessage;
	//}
}
