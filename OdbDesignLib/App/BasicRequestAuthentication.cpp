#include "BasicRequestAuthentication.h"
#include <string>
#include "macros.h"
#include <cstdlib>

using namespace Utils;

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
			if (authHeader.empty()) return crow::response(401, "Unauthorized");

			auto authValue = authHeader.substr(6);
			if (authValue.empty()) return crow::response(401, "Unauthorized");

			auto authValueDecoded = crow::utility::base64decode(authValue, authValue.size());
			if (authValueDecoded.empty()) return crow::response(401, "Unauthorized");

			auto seperatorPos = authValueDecoded.find(':');
			if (seperatorPos == std::string::npos) return crow::response(401, "Unauthorized");

			auto username = authValueDecoded.substr(0, seperatorPos);
			auto password = authValueDecoded.substr(seperatorPos + 1);

			//if (! VerifyCredentials(username, password)) return crow::response(403, "Invalid username or password");
			resp = VerifyCredentials(username, password);
		}
		return resp;
	}

	crow::response BasicRequestAuthentication::VerifyCredentials(const std::string& username, const std::string& password)
	{
		// 500 - Internal Server Error
		auto validUsername = std::getenv(USERNAME_ENV_NAME);
		if (validUsername == nullptr)	//return crow::response(500, "Failed retrieving credentials");
		{
			// default username if none supplied in environment
			validUsername = "odb";
		}

		auto validPassword = std::getenv(PASSWORD_ENV_NAME);
		if (validPassword == nullptr)	//return crow::response(500, "Failed retrieving credentials");
		{
			// default password if none supplied in environment
			validPassword = "plusplus";
		}
		
		// 403 - Forbidden
		if (username != validUsername ||
			password != validPassword)
		{
			return crow::response(403, "Invalid username or password");
		}

		// 200 Authorized!
		return crow::response(200, "Authorized");
	}
}