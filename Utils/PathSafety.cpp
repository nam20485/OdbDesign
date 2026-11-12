#include "PathSafety.h"

#include <cstdint>

namespace Utils
{
	namespace
	{
		constexpr std::size_t MAX_COMPONENT_LENGTH = 255;
		constexpr std::size_t MAX_ENTRY_PATH_LENGTH = 1024;

		bool is_control_character(unsigned char c)
		{
			return c < 0x20 || c == 0x7F;
		}

		// Split on both separators and apply a per-component policy. A component
		// must be non-empty, not "." or "..", and (optionally) component-flat-safe.
		bool split_components_all_safe(const std::string& name, bool flatPolicy)
		{
			std::size_t begin = 0;
			for (std::size_t i = 0; i <= name.size(); ++i)
			{
				const bool atSeparator = (i == name.size()) || (name[i] == '/') || (name[i] == '\\');
				if (!atSeparator)
				{
					continue;
				}

				const auto component = name.substr(begin, i - begin);
				if (component.empty())
				{
					// "//", trailing '/', or empty name
					return false;
				}
				if (component == "." || component == "..")
				{
					return false;
				}
				if (flatPolicy && !is_safe_filename_component(component))
				{
					return false;
				}

				begin = i + 1;
			}
			return true;
		}
	}

	bool is_safe_filename_component(const std::string& name)
	{
		if (name.empty() || name.size() > MAX_COMPONENT_LENGTH)
		{
			return false;
		}

		for (const auto c : name)
		{
			const auto uc = static_cast<unsigned char>(c);
			if (is_control_character(uc))
			{
				return false;
			}
			switch (c)
			{
			case '/':
			case '\\':
				// separators turn one component into a path - traversal/splitting
				return false;
			case ':':
				// drive letters ("C:x") and NTFS alternate data streams ("f.txt:stream")
				return false;
			default:
				break;
			}
		}

		// Windows strips trailing dots and spaces before hitting the filesystem,
		// so "design.tgz." and "design.tgz " would alias an existing file while
		// reading back as a different name - reject the ambiguity.
		const auto last = name.back();
		if (last == '.' || last == ' ')
		{
			return false;
		}

		return true;
	}

	bool is_safe_relative_entry_path(const std::string& name)
	{
		if (name.empty() || name.size() > MAX_ENTRY_PATH_LENGTH)
		{
			return false;
		}

		// tolerate trailing separators: directory entries are legitimately
		// stored as "step/layers/" by common tar writers - a trailing slash
		// has no traversal meaning (leading separators are still rejected
		// below, and interior empty components are rejected by the split)
		auto trimmed = name;
		while (!trimmed.empty() && (trimmed.back() == '/' || trimmed.back() == '\\'))
		{
			trimmed.pop_back();
		}
		if (trimmed.empty())
		{
			return false;
		}

		// rooted forms escape the base entirely when joined
		if (trimmed.front() == '/' || trimmed.front() == '\\')
		{
			return false;
		}

		for (const auto c : trimmed)
		{
			const auto uc = static_cast<unsigned char>(c);
			if (is_control_character(uc))
			{
				return false;
			}
			if (c == ':')
			{
				// drive forms and NTFS streams have no place in an archive entry
				return false;
			}
		}

		return split_components_all_safe(trimmed, /*flatPolicy=*/false);
	}
}
