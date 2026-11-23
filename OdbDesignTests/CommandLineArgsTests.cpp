#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommandLineArgs.h"

using ::Utils::CommandLineArgs;

namespace Odb::Test
{
    namespace
    {
        // getUsageString() is pure virtual; expose it and a couple of
        // protected helpers for direct assertions.
        class TestableCommandLineArgs : public CommandLineArgs
        {
        public:
            using CommandLineArgs::CommandLineArgs;

            std::string rawArgValue(const std::string& name) const
            {
                return getArgValue(name);
            }

            bool hasArg(const std::string& name) const
            {
                return m_mapArguments.find(name) != m_mapArguments.end();
            }

        protected:
            std::string getUsageString() const override
            {
                return "";
            }
        };
    }

    // ---- executable handling ---------------------------------------------

    TEST(CommandLineArgsTests, Executable_ProgramNameOnly)
    {
        TestableCommandLineArgs args({ "prog" });
        EXPECT_EQ(args.executable(), "prog");
        EXPECT_EQ(args.executableName(), "prog");
        EXPECT_EQ(args.executableDirectory(), "");
        EXPECT_FALSE(args.isWindows());
        EXPECT_TRUE(args.isLinux());
    }

    TEST(CommandLineArgsTests, Executable_AbsolutePath_SplitsDirectoryAndName)
    {
        TestableCommandLineArgs args({ "/usr/local/bin/odbdesign-server" });
        EXPECT_EQ(args.executable(), "/usr/local/bin/odbdesign-server");
        EXPECT_EQ(args.executableName(), "odbdesign-server");
        EXPECT_EQ(args.executableDirectory(), "/usr/local/bin");
        EXPECT_TRUE(args.isLinux());
    }

    TEST(CommandLineArgsTests, Executable_ExeExtension_DetectedAsWindows)
    {
        TestableCommandLineArgs args({ "server.exe" });
        EXPECT_TRUE(args.isWindows());
        EXPECT_FALSE(args.isLinux());
    }

    TEST(CommandLineArgsTests, Executable_ExeExtensionMatch_IsCaseSensitive)
    {
        // documented actual behavior: the ".exe" probe is a case-sensitive
        // substring match, so "SERVER.EXE" is not detected as Windows
        TestableCommandLineArgs args({ "SERVER.EXE" });
        EXPECT_FALSE(args.isWindows());
        EXPECT_TRUE(args.isLinux());
    }

    TEST(CommandLineArgsTests, Executable_ExeNotAtEnd_NotDetectedAsWindows)
    {
        TestableCommandLineArgs args({ "server.exe.bak" });
        EXPECT_FALSE(args.isWindows());
    }

    TEST(CommandLineArgsTests, ArgvConstructor_MatchesVectorConstructor)
    {
        std::vector<std::string> storage{ "prog", "--port", "1234" };
        std::vector<char*> argv;
        for (auto& s : storage)
        {
            argv.push_back(s.data());
        }

        TestableCommandLineArgs fromArgv(static_cast<int>(argv.size()), argv.data());
        TestableCommandLineArgs fromVector(storage);

        EXPECT_EQ(fromArgv.intArg("port", 0), 1234);
        EXPECT_EQ(fromVector.intArg("port", 0), 1234);
    }

    // ---- flag parsing ------------------------------------------------------

    TEST(CommandLineArgsTests, BareFlag_StoredAsTrue)
    {
        TestableCommandLineArgs args({ "prog", "--flag" });
        EXPECT_TRUE(args.hasArg("flag"));
        EXPECT_EQ(args.rawArgValue("flag"), "true");
        EXPECT_TRUE(args.boolArg("flag", false));
    }

    TEST(CommandLineArgsTests, DashStyles_SingleDoubleAndSlash_Equivalent)
    {
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "--name", "val" }).stringArg("name", ""), "val");
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "-name", "val" }).stringArg("name", ""), "val");
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "/name", "val" }).stringArg("name", ""), "val");
        // extra leading dashes are all stripped
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "---name", "val" }).stringArg("name", ""), "val");
    }

    TEST(CommandLineArgsTests, RepeatedFlag_LastOccurrenceWins)
    {
        TestableCommandLineArgs args({ "prog", "--port", "111", "--port", "222" });
        EXPECT_EQ(args.intArg("port", 0), 222);
    }

    TEST(CommandLineArgsTests, MissingFlag_AccessorsReturnDefaults)
    {
        TestableCommandLineArgs args({ "prog" });
        EXPECT_EQ(args.boolArg("missing", true), true);
        EXPECT_EQ(args.boolArg("missing", false), false);
        EXPECT_EQ(args.stringArg("missing", "fallback"), "fallback");
        EXPECT_EQ(args.intArg("missing", 8080), 8080);
        EXPECT_EQ(args.doubleArg("missing", 3.25), 3.25);
    }

    TEST(CommandLineArgsTests, EmptyStringTokens_AreSkipped)
    {
        TestableCommandLineArgs args({ "prog", "", "--flag" });
        EXPECT_TRUE(args.boolArg("flag", false));
    }

    TEST(CommandLineArgsTests, EmptyValueToken_FlagGetsTrueLiteral)
    {
        // an empty string can never be a value: the flag is stored as "true"
        TestableCommandLineArgs args({ "prog", "--flag", "" });
        EXPECT_EQ(args.rawArgValue("flag"), "true");
    }

    // KNOWN TRAP (documented workspace behavior): any token starting with '/'
    // is treated as a FLAG, so an absolute path can never be a value.
    // "--designs-dir /abs/path" stores designs-dir="true" and registers the
    // ltrimmed path ("abs/path") as a separate flag key.
    TEST(CommandLineArgsTests, AbsolutePathValue_KnownSlashPrefixTrap)
    {
        TestableCommandLineArgs args({ "prog", "--designs-dir", "/abs/path" });
        EXPECT_EQ(args.rawArgValue("designs-dir"), "true");
        EXPECT_TRUE(args.hasArg("abs/path"));
        EXPECT_FALSE(args.hasArg("/abs/path"));
    }

    // KNOWN TRAP: a value starting with '-' is likewise treated as a flag, so
    // negative numeric values are unrepresentable. "-42" is stored as flag
    // "42" and "count" keeps the boolean literal "true".
    TEST(CommandLineArgsTests, NegativeNumberValue_KnownDashPrefixTrap)
    {
        TestableCommandLineArgs args({ "prog", "--count", "-42" });
        EXPECT_EQ(args.rawArgValue("count"), "true");
        EXPECT_TRUE(args.hasArg("42"));
    }

    // documented actual behavior: "--key=value" is NOT supported; the whole
    // token becomes a flag key with the boolean literal "true"
    TEST(CommandLineArgsTests, EqualsSyntax_NotSupported_FlagKeyKeepsEquals)
    {
        TestableCommandLineArgs args({ "prog", "--port=1234" });
        EXPECT_TRUE(args.hasArg("port=1234"));
        EXPECT_FALSE(args.hasArg("port"));
        EXPECT_EQ(args.intArg("port", 5555), 5555);
    }

    // ---- typed accessors ---------------------------------------------------

    TEST(CommandLineArgsTests, IntArg_ValidValue)
    {
        TestableCommandLineArgs args({ "prog", "--count", "42" });
        EXPECT_EQ(args.intArg("count", 0), 42);
    }

    TEST(CommandLineArgsTests, IntArg_BooleanLiteral_ThrowsInvalidArgument)
    {
        // "--count" with no value stores "true"; std::stoi("true") throws.
        // Callers that do not guard this (see OdbDesignArgs::port) propagate.
        TestableCommandLineArgs args({ "prog", "--count" });
        EXPECT_THROW(args.intArg("count", 0), std::invalid_argument);
    }

    TEST(CommandLineArgsTests, IntArg_NonNumeric_ThrowsInvalidArgument)
    {
        TestableCommandLineArgs args({ "prog", "--count", "abc" });
        EXPECT_THROW(args.intArg("count", 0), std::invalid_argument);
    }

    TEST(CommandLineArgsTests, IntArg_NumericPrefix_ParsesPrefixOnly)
    {
        // documented actual behavior: std::stoi consumes the leading numeric
        // prefix and ignores the trailing garbage
        TestableCommandLineArgs args({ "prog", "--count", "12abc" });
        EXPECT_EQ(args.intArg("count", 0), 12);
    }

    TEST(CommandLineArgsTests, DoubleArg_ValidValues)
    {
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "--ratio", "2.5" }).doubleArg("ratio", 0.0), 2.5);
        EXPECT_EQ(TestableCommandLineArgs({ "prog", "--ratio", "1.5e2" }).doubleArg("ratio", 0.0), 150.0);
    }

    TEST(CommandLineArgsTests, DoubleArg_NegativeLiteral_KnownDashPrefixTrap_Throws)
    {
        // like negative ints, a negative double literal cannot be a value:
        // the parser turns "-1.5" into flag "1.5" and stores ratio="true",
        // which std::stod rejects
        TestableCommandLineArgs args({ "prog", "--ratio", "-1.5" });
        EXPECT_EQ(args.rawArgValue("ratio"), "true");
        EXPECT_TRUE(args.hasArg("1.5"));
        EXPECT_THROW(args.doubleArg("ratio", 0.0), std::invalid_argument);
    }

    TEST(CommandLineArgsTests, DoubleArg_NonNumeric_ThrowsInvalidArgument)
    {
        TestableCommandLineArgs args({ "prog", "--ratio", "abc" });
        EXPECT_THROW(args.doubleArg("ratio", 0.0), std::invalid_argument);
    }

    TEST(CommandLineArgsTests, DoubleArg_NumericPrefix_ParsesPrefixOnly)
    {
        TestableCommandLineArgs args({ "prog", "--ratio", "2.5xyz" });
        EXPECT_EQ(args.doubleArg("ratio", 0.0), 2.5);
    }

    TEST(CommandLineArgsTests, BoolArg_AlphaTrueFalse_Parsed)
    {
        EXPECT_TRUE(TestableCommandLineArgs({ "prog", "--flag", "true" }).boolArg("flag", false));
        EXPECT_FALSE(TestableCommandLineArgs({ "prog", "--flag", "false" }).boolArg("flag", true));
    }

    // KNOWN QUIRK (documented actual behavior): bool extraction uses
    // std::boolalpha, so numeric "1" does NOT parse as true — the failed
    // extraction stores false, which silently overrides the caller default.
    TEST(CommandLineArgsTests, BoolArg_NumericOne_ParsesAsFalse)
    {
        TestableCommandLineArgs args({ "prog", "--flag", "1" });
        EXPECT_FALSE(args.boolArg("flag", true));
    }

    TEST(CommandLineArgsTests, BoolArg_UppercaseTrue_ParsesAsFalse)
    {
        // boolalpha matching is against the C locale's lowercase truename
        TestableCommandLineArgs args({ "prog", "--flag", "TRUE" });
        EXPECT_FALSE(args.boolArg("flag", true));
    }
} // namespace Odb::Test
