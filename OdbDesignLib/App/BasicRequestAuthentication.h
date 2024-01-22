#pragma once

#include "IRequestAuthentication.h"

namespace Odb::Lib::App
{
	class BasicRequestAuthentication : public IRequestAuthentication
	{
	public:
		//BasicRequestAuthentication()
		//{
		//}

		// Inherited via IRequestAuthentication
		crow::response AuthenticateRequest(const crow::request& req) override;

	private:

		crow::response VerifyCredentials(const std::string& username, const std::string& password);

		const inline static char USERNAME_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_USERNAME";
		const inline static char PASSWORD_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_PASSWORD";

	};
}
