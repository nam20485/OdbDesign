#include "ProcessSpawn.h"
#include "macros.h"

#if !IS_WINDOWS
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <filesystem>

extern char** environ;
#else
#include <windows.h>
#include <filesystem>
#include <string>
#endif

namespace Utils
{
#if !IS_WINDOWS

	int run_process(const std::string& program, const std::vector<std::string>& args, bool discardOutput /* = true*/)
	{
		// Build the child's argv: argv[0] is the program by convention. posix_spawnp
		// PATH-searches the program; every argument below is a distinct argv entry,
		// so no shell ever re-parses them (audit F001).
		std::vector<char*> argv;
		argv.reserve(args.size() + 2);
		argv.push_back(const_cast<char*>(program.c_str()));
		for (const auto& arg : args)
		{
			argv.push_back(const_cast<char*>(arg.c_str()));
		}
		argv.push_back(nullptr);

		posix_spawn_file_actions_t actions;
		posix_spawn_file_actions_init(&actions);
		if (discardOutput)
		{
			// connect stdin/stdout/stderr to /dev/null inside the child
			posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
			posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
			posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
		}

		pid_t pid = -1;
		const int spawnResult = posix_spawnp(&pid, program.c_str(), &actions, nullptr, argv.data(), environ);
		posix_spawn_file_actions_destroy(&actions);

		if (spawnResult != 0)
		{
			return -1;
		}

		int status = 0;
		// EINTR: a signal during the blocking wait is not a child failure -
		// retry until waitpid gives a real verdict, else the child zombies
		// and we report a bogus "could not start" error
		int waitResult;
		do
		{
			waitResult = waitpid(pid, &status, 0);
		} while (waitResult == -1 && errno == EINTR);

		if (waitResult == -1)
		{
			return -1;
		}
		if (WIFEXITED(status))
		{
			return WEXITSTATUS(status);
		}
		return -1;
	}

#else // IS_WINDOWS

	namespace
	{
		// CreateProcessW takes one command-line STRING, not an argv array, so the
		// arguments must be quoted with the standard Windows rules (MSDN
		// "Parsing C++ Command-Line Arguments"): wrap in quotes when the argument
		// contains space/tab/quote, and double any embedded backslashes that
		// precede a quote. There is still no shell involved - cmd-style
		// metacharacters (&, |, >, ;) inside a quoted argument are literal.
		std::wstring quote_windows_argument(const std::wstring& argument)
		{
			if (!argument.empty() && argument.find_first_of(L" \t\"") == std::wstring::npos)
			{
				return argument;
			}

			std::wstring quoted;
			quoted.reserve(argument.size() + 2);
			quoted.push_back(L'"');
			std::size_t backslashRun = 0;
			for (const auto c : argument)
			{
				if (c == L'\\')
				{
					++backslashRun;
					continue;
				}
				if (c == L'"')
				{
					// double the backslashes that end a run before the quote, then
					// escape the quote itself
					quoted.append(backslashRun * 2, L'\\');
					quoted += L"\\\"";
				}
				else
				{
					quoted.append(backslashRun, L'\\');
					quoted.push_back(c);
				}
				backslashRun = 0;
			}
			quoted.append(backslashRun * 2, L'\\');
			quoted.push_back(L'"');
			return quoted;
		}
	}

	int run_process(const std::string& program, const std::vector<std::string>& args, bool discardOutput /* = true*/)
	{
		// widen via std::filesystem::path so the conversion matches however
		// the system encoded the string in the first place (path::string() on
		// Windows uses the active ANSI code page; path(string) inverts it).
		// Byte-wise wstring construction would mangle every non-ASCII path.
		// the program token must be quoted too: with lpApplicationName NULL,
		// CreateProcessW resolves the executable by whitespace-prefix
		// progression over the head of the command line, so an unquoted
		// "C:\Program Files\...\7z.exe" mis-resolves
		std::wstring commandLine = quote_windows_argument(std::filesystem::path(program).wstring());
		for (const auto& arg : args)
		{
			commandLine += L" ";
			commandLine += quote_windows_argument(std::filesystem::path(arg).wstring());
		}

		// NUL-device handles for the child's streams
		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		SECURITY_ATTRIBUTES inheritable{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
		HANDLE nullOutput = INVALID_HANDLE_VALUE;
		HANDLE nullInput = INVALID_HANDLE_VALUE;
		if (discardOutput)
		{
			startupInfo.dwFlags = STARTF_USESTDHANDLES;
			nullInput = CreateFileW(L"NUL", GENERIC_READ, 0, &inheritable, OPEN_EXISTING, 0, nullptr);
			nullOutput = CreateFileW(L"NUL", GENERIC_WRITE, 0, &inheritable, OPEN_EXISTING, 0, nullptr);
			startupInfo.hStdInput = nullInput;
			startupInfo.hStdOutput = nullOutput;
			startupInfo.hStdError = nullOutput;
		}

		PROCESS_INFORMATION processInfo{};
		const BOOL created = CreateProcessW(
			nullptr,
			commandLine.data(),
			nullptr,
			nullptr,
			discardOutput ? TRUE : FALSE,
			CREATE_NO_WINDOW,
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo);

		if (nullInput != INVALID_HANDLE_VALUE) CloseHandle(nullInput);
		if (nullOutput != INVALID_HANDLE_VALUE) CloseHandle(nullOutput);

		if (!created)
		{
			return -1;
		}

		DWORD exitCode = static_cast<DWORD>(-1);
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);

		return static_cast<int>(exitCode);
	}

#endif // IS_WINDOWS
}
