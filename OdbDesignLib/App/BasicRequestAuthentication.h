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

		// true only when BOTH credential environment variables are present and non-empty.
		// F004: when authentication is enabled and this returns false, the system must
		// fail closed (VerifyCredentials rejects everything; the server refuses to start
		// before opening any listener). There is no built-in default credential.
		static bool AreCredentialsConfigured();

		// credential environment variable names (public so deployment tooling,
		// startup validation, and tests share one source of truth)
		const inline static char USERNAME_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_USERNAME";
		const inline static char PASSWORD_ENV_NAME[] = "ODBDESIGN_SERVER_REQUEST_PASSWORD";

		// exposed for testing (fail-closed decision logic); callers normally go through AuthenticateRequest
		crow::response VerifyCredentials(const std::string& username, const std::string& password);

	private:

		const inline static char AUTHORIZATION_HEADER_NAME[] = "Authorization";
	};

}
