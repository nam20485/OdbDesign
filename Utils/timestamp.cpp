//
// Created by nam20485 on 6/12/22.
//

#include <sstream>
#include <iomanip>
#include <ctime>
#include "timestamp.h"
#include <sstream>
#include <time.h>

using namespace std::chrono;

namespace Utils
{
    const int MS_PER_S = 1000;

    std::string make_timestamp(const system_clock::time_point& timepoint)
    {
        // split the time point into FLOORED whole seconds + a non-negative
        // millisecond remainder: plain truncating to_time_t()/division makes
        // pre-epoch points render a negative millisecond field (e.g.
        // "23:59:59.-500" for epoch-1.5s instead of "23:59:58.500")
        const auto totalMs = duration_cast<milliseconds>(timepoint.time_since_epoch()).count();
        auto secCount = totalMs / MS_PER_S;
        auto msCount = totalMs % MS_PER_S;
        if (msCount < 0)
        {
            msCount += MS_PER_S;
            secCount -= 1;
        }

        const auto flooredTimepoint = system_clock::time_point(
            duration_cast<system_clock::duration>(seconds(secCount)));
        auto tp_time_t = system_clock::to_time_t(flooredTimepoint);

        struct tm tm_result;
#ifdef _WIN32
        if (localtime_s(&tm_result, &tp_time_t) != 0) return "";
#else
        if (localtime_r(&tp_time_t, &tm_result) == nullptr) return "";
#endif

        std::stringstream ss;
        // date and time
        ss << std::put_time(&tm_result, "%D %T");
        // add ms
        ss << '.' << std::setfill('0') << std::setw(3) << msCount;
        return ss.str();
    }

    std::string make_timestamp()
    {
        return make_timestamp(system_clock::now());
    }

    system_clock::time_point parse_timestamp(const std::string& timestamp, const std::string& format)
    {
        struct tm tm{};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, format.c_str());

#if defined(_DEBUG)
        std::stringstream ss2;
        ss2 << std::put_time(&tm, "%D %T");
        auto s = ss2.str();
#endif // DEBUG

        auto tp = system_clock::from_time_t(std::mktime(&tm));
        return tp;
    }
}
