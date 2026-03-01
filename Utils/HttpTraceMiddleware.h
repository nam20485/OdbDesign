#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

#include "crow.h"
#include <crow/logging.h>

namespace Utils::Tracing
{
    class HttpTraceMiddleware
    {
    public:
        struct context
        {
            std::chrono::steady_clock::time_point start;
        };

        static void SetEnabled(bool enabled) noexcept
        {
            s_enabled.store(enabled, std::memory_order_release);
        }

        void before_handle(crow::request &req, crow::response & /*res*/, context &ctx)
        {
            if (!s_enabled.load(std::memory_order_acquire))
            {
                return;
            }

            ctx.start = std::chrono::steady_clock::now();

            CROW_LOG_INFO << "[http] -> " << method_to_string(req)
                          << " " << req.url
                          << " host=" << header_or_empty(req, "Host")
                          << " from=" << remote_addr_or_unknown(req);
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx)
        {
            if (!s_enabled.load(std::memory_order_acquire))
            {
                return;
            }

            const auto end = std::chrono::steady_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - ctx.start).count();

            CROW_LOG_INFO << "[http] <- " << method_to_string(req)
                          << " " << req.url
                          << " status=" << res.code
                          << " dur_ms=" << ms
                          << " from=" << remote_addr_or_unknown(req);
        }

    private:
        static std::atomic<bool> s_enabled;

        static std::string header_or_empty(const crow::request &req, const char *header)
        {
            try
            {
                return req.get_header_value(header);
            }
            catch (...)
            {
                return {};
            }
        }

        // Best-effort remote address extraction across Crow versions.
        static std::string remote_addr_or_unknown(const crow::request &req)
        {
            // Try to get remote IP address using various Crow APIs
            // Method 1: remote_ip_address field (if available)
            // Method 2: remote_ip_address() method (if available)
            // Method 3: raw socket info from req.ci_ (crow internals)
            
            // First, try to get from headers (X-Forwarded-For, X-Real-IP)
            std::string ip = get_header_value_safe(req, "X-Forwarded-For");
            if (!ip.empty())
            {
                // X-Forwarded-For can contain multiple IPs, take the first one
                auto comma_pos = ip.find(',');
                if (comma_pos != std::string::npos)
                {
                    ip = ip.substr(0, comma_pos);
                }
                return trim(ip);
            }
            
            ip = get_header_value_safe(req, "X-Real-IP");
            if (!ip.empty())
            {
                return trim(ip);
            }
            
            // If no headers, use a placeholder (actual socket IP requires Crow internals)
            return "unknown";
        }

        static std::string get_header_value_safe(const crow::request &req, const char *header)
        {
            try
            {
                return req.get_header_value(header);
            }
            catch (...)
            {
                return {};
            }
        }

        static std::string trim(const std::string &s)
        {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return {};
            size_t end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        }

        static std::string method_to_string(const crow::request &req)
        {
            // Best-effort; if Crow doesn't have method_name(), fall back to integer.
            return method_name_if_available(req);
        }

        template <typename T>
        static auto method_name_if_available(const T &r) -> decltype(crow::method_name(r.method), std::string{})
        {
            return std::string(crow::method_name(r.method));
        }
        static std::string method_name_if_available(const crow::request &r)
        {
            return std::to_string(static_cast<int>(r.method));
        }
    };
}

inline std::atomic<bool> Utils::Tracing::HttpTraceMiddleware::s_enabled{false};
