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

		bool is_windows_reserved_device_name(const std::string& component)
		{
			// Windows resolves "NUL.tgz" to the NUL device: the name up to the
			// first dot is the device match. Reject the classic reserved set
			// (case-insensitive, per component).
			const auto stem = component.substr(0, component.find('.'));

			std::string upper;
			upper.reserve(stem.size());
			for (const auto c : stem)
			{
				upper.push_back(c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c);
			}

			if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL")
			{
				return true;
			}
			if ((upper.size() == 4) &&
				(upper.compare(0, 3, "COM") == 0 || upper.compare(0, 3, "LPT") == 0) &&
				(upper[3] >= '1' && upper[3] <= '9'))
			{
				return true;
			}
			return false;
		}

		// Full per-component policy applied identically by both validators:
		// no empty, ".", "..", Windows name-normalization aliases (".. ",
		// "file.", "file "), or reserved device names. Windows strips trailing
		// dots/spaces before hitting the filesystem, so ".. " IS ".." there -
		// a traversal that would pass a byte-equality check alone.
		bool is_safe_path_component(const std::string& component)
		{
			if (component.empty())
			{
				return false;
			}
			if (component == "." || component == "..")
			{
				return false;
			}
			const auto last = component.back();
			if (last == '.' || last == ' ')
			{
				return false;
			}
			if (is_windows_reserved_device_name(component))
			{
				return false;
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

		return is_safe_path_component(name);
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

		// apply the full per-component policy to every component (split on
		// both separators; a '\' is treated as a separator everywhere so a
		// Windows-shaped name cannot smuggle components past validation)
		std::size_t begin = 0;
		for (std::size_t i = 0; i <= trimmed.size(); ++i)
		{
			const bool atSeparator = (i == trimmed.size()) || (trimmed[i] == '/') || (trimmed[i] == '\\');
			if (!atSeparator)
			{
				continue;
			}

			if (!is_safe_path_component(trimmed.substr(begin, i - begin)))
			{
				return false;
			}
			begin = i + 1;
		}
		return true;
	}
}
