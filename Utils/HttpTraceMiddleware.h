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
            // Some Crow versions expose req.remote_ip_address (field) or req.remote_ip_address() (method)
            // or req.remote_endpoint (asio endpoint). We use SFINAE-style probes.
            if (auto ip = remote_ip_field(req); !ip.empty())
            {
                return ip;
            }
            if (auto ip = remote_ip_method(req); !ip.empty())
            {
                return ip;
            }
            if (auto ep = remote_endpoint(req); !ep.empty())
            {
                return ep;
            }
            return "unknown";
        }

        template <typename T>
        static auto remote_ip_field(const T &r) -> decltype(r.remote_ip_address, std::string{})
        {
            return r.remote_ip_address;
        }
        template <typename T>
        static std::string remote_ip_field(const T &) { return {}; }

        template <typename T>
        static auto remote_ip_method(const T &r) -> decltype(r.remote_ip_address(), std::string{})
        {
            return r.remote_ip_address();
        }
        template <typename T>
        static std::string remote_ip_method(const T &) { return {}; }

        template <typename T>
        static auto remote_endpoint(const T &r) -> decltype(r.remote_endpoint, std::string{})
        {
            std::ostringstream ss;
            ss << r.remote_endpoint;
            return ss.str();
        }
        template <typename T>
        static std::string remote_endpoint(const T &) { return {}; }

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
