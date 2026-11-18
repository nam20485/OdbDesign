#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "utils_export.h"

namespace Utils
{
	// Security-audit F001: spawn an external tool with an argv array, never a
	// shell command string. Every argument travels to the child as a single
	// argv entry, so metacharacters inside archive-derived paths (';', '$',
	// backticks, spaces...) are data, not syntax. This is the only sanctioned
	// way to run an external process on untrusted-derived inputs.
	//
	// program: searched on PATH when it contains no path separator.
	// args:    argv[1..n] passed verbatim to the child.
	// discardOutput: when true the child's stdin/stdout/stderr are connected
	//                to the null device instead of this process's streams.
	//
	// Returns the child's exit code, or -1 when the process could not be
	// started (program not found, exec failure, etc.). The return type is
	// 64-bit so Windows crash codes (e.g. 0xC0000005) stay POSITIVE and are
	// reported as command failures, not confused with the -1 spawn-failure
	// sentinel; a child killed by a signal returns 128+signal (shell
	// convention), also distinct from -1.
	std::int64_t run_process(const std::string& program, const std::vector<std::string>& args, bool discardOutput = true);
}
