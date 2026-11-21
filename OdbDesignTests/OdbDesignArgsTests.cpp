#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommandLineArgs.h"
#include "../OdbDesignLib/App/OdbDesignArgs.h"

using Odb::Lib::App::OdbDesignArgs;

namespace Odb::Test
{
    namespace
    {
        // argv-array adapter: OdbDesignArgs only exposes the (argc, argv[])
        // constructor, so tests build one from a string vector
        std::vector<char*> makeArgv(const std::vector<std::string>& storage)
        {
            std::vector<char*> argv;
            argv.reserve(storage.size());
            for (const auto& s : storage)
            {
                // cast away constness is safe: CommandLineArgs only copies
                argv.push_back(const_cast<char*>(s.c_str()));
            }
            return argv;
        }

        OdbDesignArgs makeArgs(const std::vector<std::string>& argvStrings)
        {
            auto argv = makeArgv(argvStrings);
            return OdbDesignArgs(static_cast<int>(argv.size()), argv.data());
        }

        // getUsageString() is protected; expose it for usage-text assertions
        class TestableOdbDesignArgs : public OdbDesignArgs
        {
        public:
            TestableOdbDesignArgs(int argc, char* argv[])
                : OdbDesignArgs(argc, argv)
            {
            }

            std::string usageString() const
            {
                return getUsageString();
            }
        };
    }

    // ---- every server option default ---------------------------------------

    TEST(OdbDesignArgsTests, Defaults_AllOptionsMatchDocumentedValues)
    {
        auto args = makeArgs({ "odbdesign-server" });

        EXPECT_EQ(args.port(), 8888);
        EXPECT_EQ(args.bindAddress(), "0.0.0.0");
        EXPECT_EQ(args.grpcPort(), 50051);
        EXPECT_EQ(args.designsDir(), "designs");
        EXPECT_EQ(args.templatesDir(), "templates");
        EXPECT_FALSE(args.help());
        EXPECT_FALSE(args.httpTrace());
        EXPECT_EQ(args.loadDesign(), "");
        EXPECT_FALSE(args.loadAll());
        EXPECT_FALSE(args.disableAuthentication());
        EXPECT_EQ(args.cacheMaxMb(), 4096);
        EXPECT_EQ(args.maxBackgroundLoads(), 2);
    }

    // ---- explicit options ---------------------------------------------------

    TEST(OdbDesignArgsTests, ExplicitOptions_ParsedCorrectly)
    {
        auto args = makeArgs({
            "odbdesign-server",
            "--port", "9090",
            "--bind", "127.0.0.1",
            "--grpc-port", "50052",
            "--designs-dir", "designs_rel",
            "--templates-dir", "templates_rel",
            "--load-design", "designodb_rigidflex",
            "--http-trace",
            "--load-all",
            "--disable-authentication"
        });

        EXPECT_EQ(args.port(), 9090);
        EXPECT_EQ(args.bindAddress(), "127.0.0.1");
        EXPECT_EQ(args.grpcPort(), 50052);
        EXPECT_EQ(args.designsDir(), "designs_rel");
        EXPECT_EQ(args.templatesDir(), "templates_rel");
        EXPECT_EQ(args.loadDesign(), "designodb_rigidflex");
        EXPECT_TRUE(args.httpTrace());
        EXPECT_TRUE(args.loadAll());
        EXPECT_TRUE(args.disableAuthentication());
    }

    TEST(OdbDesignArgsTests, BoolFlag_ExplicitFalse_StoredFalse)
    {
        auto args = makeArgs({ "odbdesign-server", "--load-all", "false" });
        EXPECT_FALSE(args.loadAll());
    }

    TEST(OdbDesignArgsTests, HelpFlag_PresentAndAbsent)
    {
        EXPECT_TRUE(makeArgs({ "odbdesign-server", "--help" }).help());
        EXPECT_FALSE(makeArgs({ "odbdesign-server" }).help());
    }

    // RELATIVE paths are the supported form (see CommandLineArgsTests
    // AbsolutePathValue_KnownSlashPrefixTrap): the parser treats any value
    // starting with '/' as a flag, so --designs-dir with an absolute path
    // silently falls back to the default "designs".
    TEST(OdbDesignArgsTests, DesignsDir_RelativePath_Accepted)
    {
        auto args = makeArgs({ "odbdesign-server", "--designs-dir", "OdbDesignTestData/TEST_DATA" });
        EXPECT_EQ(args.designsDir(), "OdbDesignTestData/TEST_DATA");
    }

    TEST(OdbDesignArgsTests, DesignsDir_AbsolutePath_KnownSlashPrefixTrap_StoredAsTrue)
    {
        // trap: an absolute path starts with '/', so the parser treats it as
        // a flag boundary and stores the boolean literal "true" — the server
        // would resolve its designs dir to the literal string "true", not
        // even the documented default "designs"
        auto args = makeArgs({ "odbdesign-server", "--designs-dir", "/home/user/designs" });
        EXPECT_EQ(args.designsDir(), "true");
    }

    // ---- hardened int options (cache-max-mb / max-background-loads) ---------

    TEST(OdbDesignArgsTests, CacheMaxMb_ValidValues_Parsed)
    {
        EXPECT_EQ(makeArgs({ "prog", "--cache-max-mb", "100" }).cacheMaxMb(), 100);
        // 0 is meaningful: disables LRU eviction
        EXPECT_EQ(makeArgs({ "prog", "--cache-max-mb", "0" }).cacheMaxMb(), 0);
    }

    TEST(OdbDesignArgsTests, CacheMaxMb_InvalidValue_FallsBackToDefault)
    {
        // stoi-hardened: "abc" must not terminate the process
        EXPECT_EQ(makeArgs({ "prog", "--cache-max-mb", "abc" }).cacheMaxMb(), 4096);
    }

    TEST(OdbDesignArgsTests, CacheMaxMb_MissingValue_FallsBackToDefault)
    {
        // bare "--cache-max-mb" stores the boolean literal "true" -> stoi throws -> default
        EXPECT_EQ(makeArgs({ "prog", "--cache-max-mb" }).cacheMaxMb(), 4096);
    }

    TEST(OdbDesignArgsTests, CacheMaxMb_NegativeLiteral_TrapStoresTrue_FallsBackToDefault)
    {
        // a literal "-5" starts with '-' so the parser treats it as a flag;
        // "cache-max-mb" keeps "true", stoi throws, and the default is used.
        EXPECT_EQ(makeArgs({ "prog", "--cache-max-mb", "-5" }).cacheMaxMb(), 4096);
    }

    TEST(OdbDesignArgsTests, MaxBackgroundLoads_ValidValues_Parsed)
    {
        EXPECT_EQ(makeArgs({ "prog", "--max-background-loads", "4" }).maxBackgroundLoads(), 4);
        // 0 is meaningful: unbounded concurrent background loads
        EXPECT_EQ(makeArgs({ "prog", "--max-background-loads", "0" }).maxBackgroundLoads(), 0);
    }

    TEST(OdbDesignArgsTests, MaxBackgroundLoads_InvalidValue_FallsBackToDefault)
    {
        EXPECT_EQ(makeArgs({ "prog", "--max-background-loads", "junk" }).maxBackgroundLoads(), 2);
        EXPECT_EQ(makeArgs({ "prog", "--max-background-loads" }).maxBackgroundLoads(), 2);
    }

    // KNOWN GAP (documented, not hardened like cache-max-mb): port() and
    // grpcPort() call intArg() without guarding std::invalid_argument, so a
    // malformed --port / --grpc-port throws out of the accessor.
    TEST(OdbDesignArgsTests, Port_NonNumeric_NotHardened_Throws)
    {
        auto args = makeArgs({ "prog", "--port", "abc" });
        EXPECT_THROW(args.port(), std::invalid_argument);
    }

    TEST(OdbDesignArgsTests, GrpcPort_NonNumeric_NotHardened_Throws)
    {
        auto args = makeArgs({ "prog", "--grpc-port", "abc" });
        EXPECT_THROW(args.grpcPort(), std::invalid_argument);
    }

    TEST(OdbDesignArgsTests, Port_NonNumeric_EmitsStoiExceptionFromCommandLineArgs)
    {
        // the exception escapes before any clamping/hardening can occur
        auto args = makeArgs({ "prog", "--port" }); // bare flag -> "true"
        EXPECT_THROW(args.port(), std::invalid_argument);
    }

    // ---- usage/help output --------------------------------------------------

    TEST(OdbDesignArgsTests, UsageString_ContainsAllOptionsAndDefaults)
    {
        auto argv = makeArgv({ "myserver" });
        TestableOdbDesignArgs args(static_cast<int>(argv.size()), argv.data());
        const auto usage = args.usageString();

        // program name
        EXPECT_NE(usage.find("Usage: myserver [options]"), std::string::npos);

        // every documented option flag
        for (const char* option : {
                 "--bind", "--port", "--grpc-port", "--designs-dir",
                 "--templates-dir", "--http-trace", "--load-design",
                 "--load-all", "--disable-authentication", "--cache-max-mb",
                 "--max-background-loads", "--help" })
        {
            EXPECT_NE(usage.find(option), std::string::npos) << "missing " << option;
        }

        // printed defaults must match the accessor defaults
        EXPECT_NE(usage.find("8888"), std::string::npos);
        EXPECT_NE(usage.find("50051"), std::string::npos);
        EXPECT_NE(usage.find("0.0.0.0"), std::string::npos);
        EXPECT_NE(usage.find("designs"), std::string::npos);
        EXPECT_NE(usage.find("templates"), std::string::npos);
        EXPECT_NE(usage.find("4096"), std::string::npos);
    }

    TEST(OdbDesignArgsTests, PrintUsage_WritesUsageStringToStdout)
    {
        auto argv = makeArgv({ "myserver" });
        TestableOdbDesignArgs args(static_cast<int>(argv.size()), argv.data());

        std::ostringstream captured;
        auto* oldBuf = std::cout.rdbuf(captured.rdbuf());
        args.printUsage();
        std::cout.rdbuf(oldBuf);

        EXPECT_EQ(captured.str(), args.usageString());
    }
} // namespace Odb::Test
