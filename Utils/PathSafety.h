#pragma once

#include <string>
#include "utils_export.h"

namespace Utils
{
	// Security-audit F001/F002/F003: one shared vocabulary for what an untrusted
	// name may look like before it is joined onto a trusted directory. Every
	// join of archive-, upload-, or ODB++-derived text onto a filesystem path
	// must pass one of these checks first. The checks REJECT; they never
	// transform - a rejected name is a hostile or malformed input, not
	// something to launder into a safe form.

	// A single, flat filename component (no directory structure at all):
	// no separators, no traversal, no rooted/drive forms, no control
	// characters, no trailing dots/spaces (Windows name normalization),
	// and a sane length. Use for uploaded file names and other names that
	// become exactly one path component.
	bool is_safe_filename_component(const std::string& name);

	// A relative archive-entry-style path ("step/layers/features.Z" is fine):
	// non-empty, not rooted, no drive form, no control characters, and no
	// empty/dot/dot-dot component after splitting on '/' or '\'. Interior
	// separators are legal; escaping the base directory is not. Use for
	// archive entry names and ODB++-derived nested member names.
	bool is_safe_relative_entry_path(const std::string& name);
}
