#pragma once

#include "RequestAuthenticationBase.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT BasicRequestAuthentication : public RequestAuthenticationBase
	{
	public:
		BasicRequestAuthentication(bool disableAuthentication);

		// Inherited via RequestAuthenticationBase
		crow::response AuthenticateRequest(const crow::request& req) override;

	private:

		const inline static char AUTHORIZATION_HEADER_NAME[] = "Authorization";

		crow::response VerifyCredentials(const std::string& username, const std::string& password);

		const inline static char USERNAME_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_USERNAME";
		const inline static char PASSWORD_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_PASSWORD";
		

	};
}
