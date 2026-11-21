#include "BasicRequestAuthentication.h"
#include <string>
#include <cstdlib>
#include <cctype>
#include "RequestAuthenticationBase.h"

namespace
{
	// "Basic" auth-scheme token (RFC 7235: scheme tokens are case-insensitive),
	// separated from the credentials payload by a single space
	constexpr char BASIC_SCHEME[] = "Basic";
	constexpr std::string::size_type BASIC_SCHEME_LENGTH = sizeof(BASIC_SCHEME) - 1;		// 5
	constexpr std::string::size_type SCHEME_PAYLOAD_OFFSET = BASIC_SCHEME_LENGTH + 1;	// "Basic " -> payload

	std::string getEnvValue(const char* envName)
	{
		auto szValue = std::getenv(envName);
		if (szValue == nullptr) return {};
		return std::string(szValue);
	}

	// only "Basic <payload>" is a well-formed header for this endpoint;
	// any other scheme, a bare scheme token, or a truncated header fails closed
	// as Unauthorized (previously a short header could throw from substr)
	bool isBasicAuthHeader(const std::string& headerValue)
	{
		// must be at least scheme + SP + one payload character
		if (headerValue.size() <= BASIC_SCHEME_LENGTH) return false;

		for (std::string::size_type i = 0; i < BASIC_SCHEME_LENGTH; ++i)
		{
			if (std::tolower(static_cast<unsigned char>(headerValue[i])) !=
				std::tolower(static_cast<unsigned char>(BASIC_SCHEME[i])))
			{
				return false;
			}
		}

		return headerValue[BASIC_SCHEME_LENGTH] == ' ';
	}
}

namespace Odb::Lib::App
{
	BasicRequestAuthentication::BasicRequestAuthentication(bool disableAuthentication)
		: RequestAuthenticationBase(disableAuthentication)
	{
	}

	crow::response BasicRequestAuthentication::AuthenticateRequest(const crow::request& req)
	{
		auto resp = RequestAuthenticationBase::AuthenticateRequest(req);
		if (resp.code != crow::status::OK)
		{
			const auto& authHeader = req.get_header_value(AUTHORIZATION_HEADER_NAME);
			if (authHeader.empty()) return crow::response(crow::status::UNAUTHORIZED, "Unauthorized");

			if (!isBasicAuthHeader(authHeader)) return crow::response(crow::status::UNAUTHORIZED, "Unauthorized");

			auto authValue = authHeader.substr(SCHEME_PAYLOAD_OFFSET);	// skip scheme + space
			if (authValue.empty()) return crow::response(crow::status::UNAUTHORIZED, "Unauthorized");

			auto authValueDecoded = crow::utility::base64decode(authValue, authValue.size());
			if (authValueDecoded.empty()) return crow::response(crow::status::UNAUTHORIZED, "Unauthorized");

			auto seperatorPos = authValueDecoded.find(':');
			if (seperatorPos == std::string::npos) return crow::response(crow::status::UNAUTHORIZED, "Unauthorized");

			auto username = authValueDecoded.substr(0, seperatorPos);
			auto password = authValueDecoded.substr(seperatorPos + 1);

			resp = VerifyCredentials(username, password);
		}
		return resp;
	}

	crow::response BasicRequestAuthentication::VerifyCredentials(const std::string& username, const std::string& password)
	{
		// F004 fail-closed: when authentication is enabled but credentials are not
		// configured (missing or empty environment values), reject EVERY candidate
		// credential. No built-in default ("odb"/"plusplus") may ever authorize a
		// request; the server also refuses to start in this state (see
		// OdbDesignServerApp::preServerRun), so this is defense in depth.
		if (!AreCredentialsConfigured())
		{
			// 500 - Internal Server Error (server misconfiguration, not a client fault)
			return crow::response(crow::status::INTERNAL_SERVER_ERROR, "Authentication credentials not configured");
		}

		auto validUsername = getEnvValue(USERNAME_ENV_NAME);
		auto validPassword = getEnvValue(PASSWORD_ENV_NAME);

		// 403 - Forbidden
		if (username != validUsername ||
			password != validPassword)
		{
			return crow::response(crow::status::FORBIDDEN, "Invalid username or password");
		}

		// 200 Authorized!
		return crow::response(crow::status::OK, "Authorized");
	}

	bool BasicRequestAuthentication::AreCredentialsConfigured()
	{
		return !getEnvValue(USERNAME_ENV_NAME).empty() &&
			   !getEnvValue(PASSWORD_ENV_NAME).empty();
	}
}
