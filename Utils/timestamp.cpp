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
        auto tp_time_t = system_clock::to_time_t(timepoint);        
        struct tm* p_tm = localtime(&tp_time_t);
        if (p_tm == nullptr) return "";
        auto ms = duration_cast<milliseconds>(timepoint.time_since_epoch()) % MS_PER_S;

        std::stringstream ss;
        // date and time
        ss << std::put_time(p_tm, "%D %T");
        // add ms
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
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
