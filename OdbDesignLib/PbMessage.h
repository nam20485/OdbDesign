#pragma once

#include <google/protobuf/message.h>


namespace Odb::Lib
{
	template<typename TMessage>
	class PbMessage
	{
	public:
		static std::string to_json(const TMessage& message);
		static TMessage* from_json(const std::string& json);

		// TMessage MUST derive from Message
		static_assert(std::is_base_of<google::protobuf::Message, TMessage>::value, "type parameter TMessage must derive from Message");

	private:
		PbMessage() = delete;

	};

	
	template<typename TMessage>
	std::string PbMessage<TMessage>::to_json(const TMessage& message)
	{
		std::string json;

		google::protobuf::util::JsonOptions jsonOptions;
		auto status = google::protobuf::util::MessageToJsonString(message, &json, jsonOptions);
		
		return json;
	}

	template<typename TMessage>
	TMessage* PbMessage<TMessage>::from_json(const std::string& json)
	{
		auto pMessage = new TMessage();

		google::protobuf::StringPiece sp(json);
		google::protobuf::util::JsonOptions jsonOptions;
		auto status = google::protobuf::util::JsonStringToMessage(sp, pMessage);
		
		return pMessage;
	}

}
