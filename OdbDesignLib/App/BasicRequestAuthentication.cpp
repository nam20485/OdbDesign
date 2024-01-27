#include "BasicRequestAuthentication.h"
#include <string>

namespace Odb::Lib::App
{
	crow::response BasicRequestAuthentication::AuthenticateRequest(const crow::request& req)
	{
		const auto& authHeader = req.get_header_value("Authorization");
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
		auto resp = VerifyCredentials(username, password);		
		return resp;
	}

	crow::response BasicRequestAuthentication::VerifyCredentials(const std::string& username, const std::string& password)
	{
		// 500 - Internal Server Error
		auto validUsername = std::getenv(USERNAME_ENV_NAME);
		if (validUsername == nullptr) return crow::response(500, "Server failed retrieving credentials");

		auto validPassword = std::getenv(PASSWORD_ENV_NAME);
		if (validPassword == nullptr) return crow::response(500, "Server failed retrieving credentials");
		
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