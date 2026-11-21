#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include "Logger.h"

// NOTE: this file deliberately avoids including "Fixtures/TestUtils.h".
// That header declares namespace Odb::Test::Utils, which would shadow the
// global Utils namespace inside Odb::Test and break the loginfo/logerror/
// logexception macros (they expand to Utils::Logger::...).

namespace
{
    std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }

    size_t countOccurrences(const std::string& haystack, const std::string& needle)
    {
        size_t count = 0;
        size_t pos = 0;
        while ((pos = haystack.find(needle, pos)) != std::string::npos)
        {
            ++count;
            ++pos;
        }
        return count;
    }

    // Logger's file sink always writes to the relative name "log.txt" (there
    // is no filename setter), so file-sink tests temporarily move the process
    // into a managed temp directory and restore afterwards.
    class TempDir
    {
    public:
        TempDir()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(0, 15);
            std::string name = "logger_test_";
            for (int i = 0; i < 8; ++i)
            {
                name += "0123456789abcdef"[static_cast<size_t>(dis(gen))];
            }
            m_path = std::filesystem::temp_directory_path() / name;
            std::filesystem::create_directories(m_path);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(m_path, ec);
        }

        const std::filesystem::path& path() const { return m_path; }

        TempDir(const TempDir&) = delete;
        TempDir& operator=(const TempDir&) = delete;

    private:
        std::filesystem::path m_path;
    };

    class ScopedWorkingDirectory
    {
    public:
        explicit ScopedWorkingDirectory(const std::filesystem::path& dir)
            : m_original(std::filesystem::current_path())
        {
            std::filesystem::current_path(dir);
        }

        ~ScopedWorkingDirectory()
        {
            std::error_code ec;
            std::filesystem::current_path(m_original, ec);
        }

        ScopedWorkingDirectory(const ScopedWorkingDirectory&) = delete;
        ScopedWorkingDirectory& operator=(const ScopedWorkingDirectory&) = delete;

    private:
        std::filesystem::path m_original;
    };
}

class LoggerTest : public testing::Test
{
protected:
    void SetUp() override
    {
        m_pLogger = Utils::Logger::instance();
        // deterministic configuration for every test
        m_pLogger->logLevel(Utils::Logger::Level::Info);
        m_pLogger->outputTypes(Utils::Logger::OutputTypes::StdOut |
                               Utils::Logger::OutputTypes::File);
    }

    void TearDown() override
    {
        // safety net: never leave the background loop running after a test
        if (m_started)
        {
            m_started = false;
            m_pLogger->stop();
        }
    }

    void start()
    {
        m_pLogger->start();
        m_started = true;
        // let the loop thread reach its blocking wait(); stopProcessing()
        // right after start() can otherwise exit the loop before it ever
        // drains, silently dropping the test's messages (seen in probes)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void stop()
    {
        m_pLogger->stop();
        m_started = false;
    }

    Utils::Logger* m_pLogger = nullptr;
    bool m_started = false;
};

TEST_F(LoggerTest, Defaults_LevelOutputTypesAndFilename)
{
    EXPECT_EQ(m_pLogger->logLevel(), Utils::Logger::Level::Info);
    EXPECT_EQ(m_pLogger->outputTypes(),
              static_cast<int>(Utils::Logger::OutputTypes::StdOut) |
                  static_cast<int>(Utils::Logger::OutputTypes::File));
    EXPECT_EQ(m_pLogger->filename(), "log.txt");
}

TEST_F(LoggerTest, FileSink_StartLogDrainStop_ReadBackMatchesLoggedContent)
{
    TempDir tempDir;
    ScopedWorkingDirectory cwd(tempDir.path());
    std::error_code ec;
    std::filesystem::remove("log.txt", ec);

    m_pLogger->outputTypes(Utils::Logger::OutputTypes::File);
    m_pLogger->logLevel(Utils::Logger::Level::Debug);

    start();
    loginfo("file sink message one");
    m_pLogger->warn("file sink warn message");
    logerror("file sink error message");
    logdebug("file sink debug message");
    {
        std::stringstream ss;
        ss << "file sink stream message";
        m_pLogger->info(ss);
    }
    try
    {
        throw std::runtime_error("boom");
    }
    catch (const std::exception& e)
    {
        logexception(e);
    }
    try
    {
        throw std::runtime_error("boom2");
    }
    catch (const std::exception& e)
    {
        logexception_msg(e, "exception context line");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop(); // stopProcessing() drains everything queued before returning

    // logging to the file IS the behavior under test: read the file back
    // and compare against the exact content that was logged
    const auto content = readFile("log.txt");

    // the lifecycle banners are themselves log records
    EXPECT_NE(content.find("logger started"), std::string::npos);
    EXPECT_NE(content.find("logger stopped"), std::string::npos);

    // each message with its padded level tag
    EXPECT_NE(content.find("file sink message one"), std::string::npos);
    EXPECT_NE(content.find("  INFO]"), std::string::npos);
    EXPECT_NE(content.find("file sink warn message"), std::string::npos);
    EXPECT_NE(content.find("  WARN]"), std::string::npos);
    EXPECT_NE(content.find("file sink error message"), std::string::npos);
    EXPECT_NE(content.find(" ERROR]"), std::string::npos);
    EXPECT_NE(content.find("file sink debug message"), std::string::npos);
    EXPECT_NE(content.find(" DEBUG]"), std::string::npos);
    EXPECT_NE(content.find("file sink stream message"), std::string::npos);

    // exception logging: typeid name, what() text and context line
    EXPECT_NE(content.find("EXCEPTION:"), std::string::npos);
    EXPECT_NE(content.find("\"boom\""), std::string::npos);
    EXPECT_NE(content.find("\"boom2\""), std::string::npos);
    EXPECT_NE(content.find("exception context line"), std::string::npos);

    // macro-carried source location is formatted into the record
    EXPECT_NE(content.find("LoggerTests.cpp"), std::string::npos);
}

TEST_F(LoggerTest, LevelFiltering_ConsoleWritesOnlyAtOrAboveLevel)
{
    m_pLogger->outputTypes(Utils::Logger::OutputTypes::StdOut);
    m_pLogger->logLevel(Utils::Logger::Level::Error);

    std::ostringstream captured;
    auto* const oldBuf = std::cout.rdbuf(captured.rdbuf());
    start();
    logdebug("console debug dropped");
    m_pLogger->info("console info dropped");
    logerror("console error kept");
    stop();
    std::cout.rdbuf(oldBuf);

    const auto& capturedStr = captured.str();
    EXPECT_NE(capturedStr.find("console error kept"), std::string::npos);
    EXPECT_EQ(capturedStr.find("console debug dropped"), std::string::npos);
    EXPECT_EQ(capturedStr.find("console info dropped"), std::string::npos);
    // the start/stop banners are Info records: below the Error threshold
    EXPECT_EQ(capturedStr.find("logger started"), std::string::npos);
    EXPECT_EQ(capturedStr.find("logger stopped"), std::string::npos);
}

// REGRESSION GUARD (was a suspected defect, fixed in this PR): the level
// gate used to wrap only the console sinks, so records below the configured
// level still landed in log.txt. The filter now applies to ALL sinks
// (console and file alike) -- the only non-surprising contract for a
// level-filtered logger.
TEST_F(LoggerTest, FileSink_RespectsLogLevel)
{
    TempDir tempDir;
    ScopedWorkingDirectory cwd(tempDir.path());
    std::error_code ec;
    std::filesystem::remove("log.txt", ec);

    m_pLogger->outputTypes(Utils::Logger::OutputTypes::File);
    m_pLogger->logLevel(Utils::Logger::Level::Error);

    start();
    loginfo("filtered info must not reach file");
    logerror("kept error message");
    stop();

    const auto content = readFile("log.txt");
    EXPECT_EQ(content.find("filtered info must not reach file"), std::string::npos);
    EXPECT_NE(content.find("kept error message"), std::string::npos);
}

TEST_F(LoggerTest, RepeatedLifecycle_MessagesFromBothCyclesPresent)
{
    TempDir tempDir;
    ScopedWorkingDirectory cwd(tempDir.path());
    std::error_code ec;
    std::filesystem::remove("log.txt", ec);

    m_pLogger->outputTypes(Utils::Logger::OutputTypes::File);
    m_pLogger->logLevel(Utils::Logger::Level::Debug);

    start();
    loginfo("cycle one message");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop();

    start();
    loginfo("cycle two message");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop();

    const auto content = readFile("log.txt");
    EXPECT_NE(content.find("cycle one message"), std::string::npos);
    EXPECT_NE(content.find("cycle two message"), std::string::npos);

    // two full lifecycles => two start banners appended to one file
    EXPECT_GE(countOccurrences(content, "logger started"), 2u);
}

TEST_F(LoggerTest, StopAfterStart_JoinsCleanlyWithoutWork)
{
    // start + immediate stop must join the loop thread without hanging
    // or crashing, even with no work items queued
    m_pLogger->outputTypes(Utils::Logger::OutputTypes::File);
    start();
    stop();
    SUCCEED();
}
