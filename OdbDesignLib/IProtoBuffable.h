#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include "crow_win.h"
#include "IJsonable.h"
#include "odbdesign_export.h"
#include <ostream>
#include <istream>
#include <absl/strings/string_view.h>


namespace Odb::Lib
{
	template<typename TPbMessage>
	class IProtoBuffable : public Utils::IJsonable
	{
	public:		
		// to and from Protocol Buffer messages
		virtual std::unique_ptr<TPbMessage> to_protobuf() const = 0;
		virtual void from_protobuf(const TPbMessage& message) = 0;

		// to and from Protocol Buffer string
		bool to_pbstring(std::string& pb_string) const;
		bool from_pbstring(const std::string& pb_string);

		// to and from output and input streams
		bool to_stream(std::ostream& outputStream) const;
		bool from_stream(std::istream& inputStream);

		// Inherited via IJsonConvertable
		std::string to_json() const override;
		void from_json(const std::string& json) override;	
				
	protected:
		// abstract class
		IProtoBuffable() = default;

		// TMessage MUST derive from Message (must use this until template type contraints support is added)
		static_assert(std::is_base_of<google::protobuf::Message, TPbMessage>::value, "template parameter type TPbMessage must derive from Protobuf Message class");
		
	};

	template<typename TPbMessage>
	inline bool IProtoBuffable<TPbMessage>::to_pbstring(std::string& pb_string) const
	{
		auto pMessage = to_protobuf();
		if (pMessage == nullptr) return false;
		return pMessage->SerializeToString(&pb_string);
	}

	template<typename TPbMessage>
	inline bool IProtoBuffable<TPbMessage>::from_pbstring(const std::string& pb_string)
	{
		auto pMessage = std::unique_ptr<TPbMessage>();
		if (!pMessage->ParseFromString(pb_string)) return false;
		from_protobuf(*pMessage);
		return true;
	}

	template<typename TPbMessage>
	inline bool IProtoBuffable<TPbMessage>::to_stream(std::ostream& outputStream) const
	{
		auto pMessage = to_protobuf();
		if (pMessage == nullptr) return false;
		return pMessage->SerializeToOstream(&outputStream);
	}

	template<typename TPbMessage>
	inline bool IProtoBuffable<TPbMessage>::from_stream(std::istream& inputStream)
	{
		auto pMessage = std::unique_ptr<TPbMessage>();
		if (!pMessage->ParseFromIstream(&inputStream)) return false;
		from_protobuf(*pMessage);
		return true;
	}

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
		// use default options
		google::protobuf::util::JsonOptions jsonOptions;

		auto pMessage = std::unique_ptr<TPbMessage>();
		auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(json), pMessage.get());
		if (!status.ok()) return;		
		from_protobuf(*pMessage);		
	}	
}
