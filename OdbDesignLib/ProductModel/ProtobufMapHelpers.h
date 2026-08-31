#pragma once

#include <string>
#include "../Text/Utf8Sanitizer.h"
#include "Logger.h"

namespace Odb::Lib::ProductModel
{
	// Copies entries into a protobuf map<string, TMessage> field, sanitizing each
	// key with Odb::Lib::Text::ToUtf8. Distinct raw keys can sanitize to the same
	// UTF-8 key — most commonly through the five undefined CP1252 slots (0x81,
	// 0x8D, 0x8F, 0x90, 0x9D), which all map to U+FFFD, but also when a repaired
	// key ("A\xC3" -> "AÃ") matches an already-valid UTF-8 key, or a key already
	// containing U+FFFD matches one whose undefined byte mapped to U+FFFD. The
	// find() check handles every collision uniformly: the first entry wins and
	// the collision is logged, so no entry is ever silently overwritten.
	template <typename TPbMap, typename TEntries>
	void FillProtobufMapWithSanitizedKeys(TPbMap& pbMap, const TEntries& entries)
	{
		for (const auto& kv : entries)
		{
			auto sanitizedKey = Odb::Lib::Text::ToUtf8(kv.first);
			if (pbMap.find(sanitizedKey) != pbMap.end())
			{
				logwarn("UTF-8 sanitization collision in protobuf map: key \"" + kv.first +
					"\" sanitizes to a key that is already present; keeping the first entry");
				continue;
			}
			pbMap[sanitizedKey] = *kv.second->to_protobuf();
		}
	}
}
