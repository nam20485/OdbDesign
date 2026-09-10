#include <gtest/gtest.h>
#include "Fixtures/TestUtils.h"
#include <ETag.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace Odb::Test::Utils;
using namespace Utils;

namespace Odb::Test::HttpCaching
{
    // =========================================================================
    // HTTP caching (M1.3) unit tests
    // =========================================================================
    // Covers the ETag helper (Utils/ETag.h): digest determinism and field
    // sensitivity, quoted-hex formatting, archive-file discovery by stem,
    // and the mtime/size invalidation contract (replacing an archive changes
    // the tag) against real files in a temp designs directory.
    //
    // The If-None-Match -> 304 header behavior of the live routes is wired in
    // RouteController::checkConditionalGet and FileModelController; it needs a
    // running Crow server to exercise end-to-end and is verified manually
    // (no existing test harness boots the server).
    // =========================================================================

    class HttpCachingTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_designsDir = TestUtils::createManagedTempDirectory("etag_designs");
        }

        void TearDown() override
        {
            m_designsDir.reset();
        }

        std::string designsDir() const
        {
            return m_designsDir->path().string();
        }

        // Writes <designsDir>/<filename> with content and pins its mtime to
        // (file_time_type epoch + mtimeSeconds), so tests control the tag
        // inputs exactly and never depend on filesystem timestamp granularity.
        void writeDesignFile(const std::string& filename, const std::string& content, std::int64_t mtimeSeconds)
        {
            auto path = m_designsDir->path() / filename;
            std::ofstream file(path, std::ios::binary);
            file.write(content.c_str(), static_cast<std::streamsize>(content.size()));
            file.close();
            ASSERT_TRUE(std::filesystem::exists(path));

            const auto mtime = std::filesystem::file_time_type() + std::chrono::seconds(mtimeSeconds);
            std::filesystem::last_write_time(path, mtime);
        }

        std::unique_ptr<TestUtils::TempResource> m_designsDir;
    };

    // ---- MakeEtag: format, determinism, field sensitivity ----

    TEST_F(HttpCachingTest, MakeEtag_FormatIsQuotedLowercaseHex)
    {
        auto tag = MakeEtag("design1", 1000, 12345, "/filemodels/<string>");

        ASSERT_EQ(tag.size(), 18);  // 2 quotes + 16 hex digits
        EXPECT_EQ(tag.front(), '"');
        EXPECT_EQ(tag.back(), '"');
        for (auto i = 1u; i < tag.size() - 1; ++i)
        {
            const auto& c = tag[i];
            EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) << "char at " << i << ": " << c;
        }
    }

    TEST_F(HttpCachingTest, MakeEtag_IsDeterministic)
    {
        EXPECT_EQ(MakeEtag("design1", 1000, 12345, "/filemodels/<string>"),
                  MakeEtag("design1", 1000, 12345, "/filemodels/<string>"));
    }

    TEST_F(HttpCachingTest, MakeEtag_VariesByDesignName)
    {
        EXPECT_NE(MakeEtag("design1", 1000, 12345, "/filemodels/<string>"),
                  MakeEtag("design2", 1000, 12345, "/filemodels/<string>"));
    }

    TEST_F(HttpCachingTest, MakeEtag_VariesByMtime)
    {
        EXPECT_NE(MakeEtag("design1", 1000, 12345, "/filemodels/<string>"),
                  MakeEtag("design1", 2000, 12345, "/filemodels/<string>"));
    }

    TEST_F(HttpCachingTest, MakeEtag_VariesBySize)
    {
        EXPECT_NE(MakeEtag("design1", 1000, 12345, "/filemodels/<string>"),
                  MakeEtag("design1", 1000, 54321, "/filemodels/<string>"));
    }

    TEST_F(HttpCachingTest, MakeEtag_VariesByEndpointPath)
    {
        EXPECT_NE(MakeEtag("design1", 1000, 12345, "/filemodels/<string>"),
                  MakeEtag("design1", 1000, 12345, "/filemodels/<string>/matrix/matrix"));
    }

    // ---- FindDesignArchiveFile: stem matching over the DesignCache extension set ----

    TEST_F(HttpCachingTest, FindDesignArchiveFile_FindsEachSupportedExtension)
    {
        const std::vector<std::pair<std::string, std::string>> cases = {
            {"simple_tgz", ".tgz"},
            {"simple_zip", ".zip"},
            {"simple_tar", ".tar"},
            {"simple_gzip", ".gzip"},
            {"simple_gz", ".gz"},
        };
        for (const auto& [stem, extension] : cases)
        {
            writeDesignFile(stem + extension, "x", 1000);
            auto found = FindDesignArchiveFile(designsDir(), stem);
            EXPECT_FALSE(found.empty()) << "extension: " << extension;
        }
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_ExtensionMatchIsCaseInsensitive)
    {
        writeDesignFile("MYDESIGN.TGZ", "x", 1000);

        auto found = FindDesignArchiveFile(designsDir(), "MYDESIGN");
        ASSERT_FALSE(found.empty());
        EXPECT_EQ(found.filename().string(), "MYDESIGN.TGZ");
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_MatchesMultiPartTarGzExtension)
    {
        writeDesignFile("tarborn.tgz", "x", 1000);
        writeDesignFile("flexi.tar.gz", "x", 1000);

        // ".tar.gz" must strip whole (design "flexi"), not leave "flexi.tar"
        auto found = FindDesignArchiveFile(designsDir(), "flexi");
        ASSERT_FALSE(found.empty());
        EXPECT_EQ(found.filename().string(), "flexi.tar.gz");
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_UnknownName_ReturnsEmpty)
    {
        writeDesignFile("design1.tgz", "x", 1000);

        EXPECT_TRUE(FindDesignArchiveFile(designsDir(), "nosuchdesign").empty());
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_UnsupportedExtension_ReturnsEmpty)
    {
        writeDesignFile("notes.txt", "x", 1000);

        EXPECT_TRUE(FindDesignArchiveFile(designsDir(), "notes").empty());
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_MissingDirectory_ReturnsEmpty)
    {
        EXPECT_TRUE(FindDesignArchiveFile((m_designsDir->path() / "does_not_exist").string(), "design1").empty());
    }

    // ---- MakeDesignEtag: stat-driven tag + invalidation contract ----

    TEST_F(HttpCachingTest, MakeDesignEtag_NoArchiveFile_ReturnsEmpty)
    {
        EXPECT_EQ(MakeDesignEtag(designsDir(), "nosuchdesign", "/filemodels/<string>"), "");
    }

    TEST_F(HttpCachingTest, MakeDesignEtag_MatchesRawFieldsFromStat)
    {
        writeDesignFile("design1.tgz", "archive-bytes", 1234);

        const auto path = m_designsDir->path() / "design1.tgz";
        const auto mtimeTicks = static_cast<std::int64_t>(std::filesystem::last_write_time(path).time_since_epoch().count());
        const auto sizeBytes = std::filesystem::file_size(path);

        EXPECT_EQ(MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>"),
                  MakeEtag("design1", mtimeTicks, sizeBytes, "/filemodels/<string>"));
    }

    // Invalidation contract: replacing/re-uploading the archive changes the
    // tag (mtime + size are part of the digest), so clients revalidating with
    // the stale If-None-Match get a fresh 200 without any explicit
    // invalidation step.
    TEST_F(HttpCachingTest, MakeDesignEtag_ReplacedArchive_ChangesTag)
    {
        writeDesignFile("design1.tgz", "original archive contents", 1000);
        const auto etagBefore = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>");
        ASSERT_FALSE(etagBefore.empty());

        // Re-upload: different content (size) and a bumped mtime
        writeDesignFile("design1.tgz", "re-uploaded, larger archive contents!!", 2000);
        const auto etagAfter = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>");

        EXPECT_NE(etagBefore, etagAfter);
    }

    TEST_F(HttpCachingTest, MakeDesignEtag_TouchOnlyMtime_ChangesTag)
    {
        writeDesignFile("design1.tgz", "stable contents", 1000);
        const auto etagBefore = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>");

        writeDesignFile("design1.tgz", "stable contents", 5000);
        const auto etagAfter = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>");

        EXPECT_NE(etagBefore, etagAfter);
    }

    TEST_F(HttpCachingTest, MakeDesignEtag_SameFileDifferentEndpoint_Differs)
    {
        writeDesignFile("design1.tgz", "x", 1000);

        EXPECT_NE(MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>/matrix/matrix"),
                  MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>/misc/info"));
    }

    // ---- IfNoneMatchMatches: RFC 7232 weak comparison ----

    TEST_F(HttpCachingTest, IfNoneMatchMatches_ExactTag_Matches)
    {
        EXPECT_TRUE(IfNoneMatchMatches("\"a1b2c3d4e5f60718\"", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_ListContainingTag_Matches)
    {
        EXPECT_TRUE(IfNoneMatchMatches("\"0000000000000000\", \"a1b2c3d4e5f60718\"", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_WeakPrefix_Matches)
    {
        // weak comparison: If-None-Match ignores the W/ prefix when comparing
        EXPECT_TRUE(IfNoneMatchMatches("W/\"a1b2c3d4e5f60718\"", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_Star_MatchesAny)
    {
        EXPECT_TRUE(IfNoneMatchMatches("*", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_DifferentTag_DoesNotMatch)
    {
        EXPECT_FALSE(IfNoneMatchMatches("\"0000000000000000\"", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_EmptyHeader_DoesNotMatch)
    {
        EXPECT_FALSE(IfNoneMatchMatches("", "\"a1b2c3d4e5f60718\""));
    }

    TEST_F(HttpCachingTest, IfNoneMatchMatches_UnquotedJunk_DoesNotMatch)
    {
        EXPECT_FALSE(IfNoneMatchMatches("a1b2c3d4e5f60718", "\"a1b2c3d4e5f60718\""));
    }
}
