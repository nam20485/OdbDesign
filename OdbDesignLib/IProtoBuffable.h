#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <crow.h>

using namespace google::protobuf;
using namespace google::protobuf::util;


namespace Odb::Lib
{
	template<typename TMessage>
	class IProtoBuffable// : crow::returnable
	{
	public:
		virtual TMessage* to_protobuf() const = 0;
		virtual void from_protobuf(const TMessage& message) = 0;

		std::string to_json() const;
		//void from_json(const std::string& json);
		static TMessage* from_json(const std::string& json);

	protected:
		IProtoBuffable()
			//: returnable("application/json")
		{}

		// TMessage MUST derive from Message
		static_assert(std::is_base_of<Message, TMessage>::value, "type parameter TMessage must derive from Message class");

		//// Inherited via returnable
		//std::string dump() const override
		//{
		//	return to_json();
		//}

	};

	template<typename TMessage>
	std::string IProtoBuffable<TMessage>::to_json() const
	{
		std::string json;

		google::protobuf::util::JsonOptions jsonOptions;
		auto status = MessageToJsonString(*(static_cast<Message*>(this->to_protobuf())), &json, jsonOptions);

		return json;
	}

	template<typename TMessage>
	TMessage* IProtoBuffable<TMessage>::from_json(const std::string& json)
	{
		auto pMessage = new TMessage();

		google::protobuf::StringPiece sp(json);
		google::protobuf::util::JsonOptions jsonOptions;
		auto status = google::protobuf::util::JsonStringToMessage(sp, pMessage);

		return pMessage;
	}
}
