//
// Created by nam20485 on 6/12/22.
//

#include <sstream>
#include <iomanip>
#include <ctime>
#include "timestamp.h"

namespace Utils
{
    std::string make_timestamp(const std::chrono::system_clock::time_point& timepoint)
    {
        auto now_time_t = std::chrono::system_clock::to_time_t(timepoint);
        struct tm now_tm;
        if (0 != localtime_s(&now_tm, &now_time_t)) return "";
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(&now_tm, "%D %T");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string make_timestamp()
    {
        return make_timestamp(std::chrono::system_clock::now());
    }

    std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp)
    {
        return std::chrono::system_clock::time_point();
    }
}
