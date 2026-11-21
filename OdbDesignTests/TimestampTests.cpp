#include <gtest/gtest.h>
#include <chrono>
#include <ctime>
#include <regex>
#include <string>

#include "timestamp.h"

using namespace std::chrono;

namespace Odb::Test
{
    namespace
    {
        std::chrono::system_clock::time_point timePointFromMs(int64_t msSinceEpoch)
        {
            return std::chrono::system_clock::time_point(
                std::chrono::duration_cast<system_clock::duration>(milliseconds(msSinceEpoch)));
        }

        // localtime_r is POSIX-only; MSVC/UCRT provides localtime_s with a
        // different argument order (same guard as Utils/timestamp.cpp)
        void toLocalTm(std::tm& out, std::time_t timeT)
        {
#ifdef _WIN32
            localtime_s(&out, &timeT);
#else
            localtime_r(&timeT, &out);
#endif
        }
    }

    TEST(TimestampTests, MakeTimestamp_FormatMatchesDdMmYyHhMmSsMmm)
    {
        const auto stamp = ::Utils::make_timestamp();
        // "%D %T" plus ".mmm": MM/DD/YY HH:MM:SS.mmm under the C locale
        const std::regex pattern(R"(^\d{2}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}\.\d{3}$)");
        EXPECT_TRUE(std::regex_match(stamp, pattern)) << "actual: " << stamp;
    }

    TEST(TimestampTests, MakeTimestamp_KnownEpoch_MillisecondPrecision)
    {
        // 1234.567s after the epoch: the millisecond part is timezone-independent
        const auto stamp = ::Utils::make_timestamp(timePointFromMs(1234567));
        EXPECT_EQ(stamp.substr(stamp.size() - 4), ".567");
    }

    TEST(TimestampTests, MakeTimestamp_ExactSecond_ZeroPaddedMilliseconds)
    {
        const auto stamp = ::Utils::make_timestamp(timePointFromMs(0));
        EXPECT_EQ(stamp.substr(stamp.size() - 4), ".000");
    }

    // REGRESSION GUARD (was a known defect, fixed in this PR): the
    // millisecond remainder used to come from truncating division, rendering
    // pre-epoch points with a negative millisecond field ("...59.-500").
    // make_timestamp() now floors the seconds and normalizes the remainder,
    // so epoch-1.5s renders "...58.500".
    TEST(TimestampTests, MakeTimestamp_PreEpoch_MillisecondPartIsNonNegative)
    {
        const auto stamp = ::Utils::make_timestamp(timePointFromMs(-1500));
        EXPECT_EQ(stamp.substr(stamp.size() - 4), ".500")
            << "actual: " << stamp;
    }

    TEST(TimestampTests, ParseTimestamp_RoundTrip_FromMakeTimestamp_SecondsGranularity)
    {
        const auto original = system_clock::now();
        const auto stamp = ::Utils::make_timestamp(original);
        const auto parsed = ::Utils::parse_timestamp(stamp, "%D %T");
        EXPECT_EQ(system_clock::to_time_t(parsed), system_clock::to_time_t(original));
    }

    TEST(TimestampTests, ParseTimestamp_KnownComponents_LocalTimeRoundTrip)
    {
        const auto parsed = ::Utils::parse_timestamp("12/25/23 15:45:30", "%D %T");
        std::tm tmResult{};
        const auto timeT = system_clock::to_time_t(parsed);
        toLocalTm(tmResult, timeT);
        // assert against localtime so the test is timezone-independent
        EXPECT_EQ(tmResult.tm_year, 123); // years since 1900
        EXPECT_EQ(tmResult.tm_mon, 11);   // 0-based: December
        EXPECT_EQ(tmResult.tm_mday, 25);
        EXPECT_EQ(tmResult.tm_hour, 15);
        EXPECT_EQ(tmResult.tm_min, 45);
        EXPECT_EQ(tmResult.tm_sec, 30);
    }

    TEST(TimestampTests, ParseTimestamp_InvalidInput_DoesNotThrow)
    {
        // documented actual behavior: get_time()'s failbit is ignored, so an
        // unparseable string yields whatever the zeroed tm normalizes to
        // instead of an error
        EXPECT_NO_THROW(::Utils::parse_timestamp("not a timestamp", "%D %T"));
    }
} // namespace Odb::Test
