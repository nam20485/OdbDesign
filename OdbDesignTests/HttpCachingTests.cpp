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

    TEST_F(HttpCachingTest, MakeEtag_VariesByGeneration)
    {
        // The cache invalidation generation is part of the digest so
        // in-memory-only changes (POST /filemodels with save=false) rotate
        // the tag without touching the archive file on disk.
        EXPECT_NE(MakeEtag("design1", 1000, 12345, "/filemodels/<string>", 0),
                  MakeEtag("design1", 1000, 12345, "/filemodels/<string>", 1));
        EXPECT_EQ(MakeEtag("design1", 1000, 12345, "/filemodels/<string>", 7),
                  MakeEtag("design1", 1000, 12345, "/filemodels/<string>", 7));
    }

    // ---- FindDesignArchiveFile: stem matching, mirroring DesignCache serving ----

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

    TEST_F(HttpCachingTest, FindDesignArchiveFile_MatchesByStemRegardlessOfExtensionCase)
    {
        writeDesignFile("MYDESIGN.TGZ", "x", 1000);

        auto found = FindDesignArchiveFile(designsDir(), "MYDESIGN");
        ASSERT_FALSE(found.empty());
        EXPECT_EQ(found.filename().string(), "MYDESIGN.TGZ");
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_MultiPartTarGzFollowsStemSemantics)
    {
        writeDesignFile("tarborn.tgz", "x", 1000);
        writeDesignFile("flexi.tar.gz", "x", 1000);

        // Matching mirrors DesignCache::FindArchivePath exactly: a file's stem
        // strips only the LAST extension, so "flexi.tar.gz" belongs to design
        // "flexi.tar" — the same name the server serves and uploads it under.
        // A whole-extension strip here would let the validator attest a
        // different archive than the one behind the body.
        EXPECT_TRUE(FindDesignArchiveFile(designsDir(), "flexi").empty());
        auto found = FindDesignArchiveFile(designsDir(), "flexi.tar");
        ASSERT_FALSE(found.empty());
        EXPECT_EQ(found.filename().string(), "flexi.tar.gz");
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_UnknownName_ReturnsEmpty)
    {
        writeDesignFile("design1.tgz", "x", 1000);

        EXPECT_TRUE(FindDesignArchiveFile(designsDir(), "nosuchdesign").empty());
    }

    TEST_F(HttpCachingTest, FindDesignArchiveFile_MatchesAnyExtensionLikeDesignCache)
    {
        // DesignCache::FindArchivePath serves the first regular file whose stem
        // matches, with no extension whitelist; the validator must resolve the
        // identical file or the tag could attest an archive the server would
        // never serve for that name.
        writeDesignFile("notes.txt", "x", 1000);

        auto found = FindDesignArchiveFile(designsDir(), "notes");
        ASSERT_FALSE(found.empty());
        EXPECT_EQ(found.filename().string(), "notes.txt");
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

    // Generation invalidation contract: a cache-side replacement that never
    // touches the archive file (POST /filemodels with save=false) still
    // rotates the tag, so a revalidating client gets a fresh 200 instead of
    // a 304 describing data it already has.
    TEST_F(HttpCachingTest, MakeDesignEtag_VariesByGeneration_SameFileOnDisk)
    {
        writeDesignFile("design1.tgz", "stable archive contents", 1000);

        const auto before = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>", 3);
        const auto after = MakeDesignEtag(designsDir(), "design1", "/filemodels/<string>", 4);

        ASSERT_FALSE(before.empty());
        EXPECT_NE(before, after);
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
