#include "RequestAuthenticationBase.h"
#include "macros.h"

using namespace Utils;

namespace Odb::Lib::App
{	
	RequestAuthenticationBase::RequestAuthenticationBase(bool disableAuthentication)
		: m_disableAuthentication(disableAuthentication)
	{
	}	

	crow::response RequestAuthenticationBase::AuthenticateRequest(const crow::request& req)
	{
		// if running debug build AND in Local environment, bypass authentication
		if (IsDebug() && IsLocal())
		{
			// 200 Authorized!
			return crow::response(200, "Authorized");
		}
		else if (m_disableAuthentication)
		{
			// 200 Authorized!
			return crow::response(200, "Authorized");
		}
		else
		{
			return crow::response(401, "Unauthorized");
		}
	}
}