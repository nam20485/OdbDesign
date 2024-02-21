#pragma once

#include "../odbdesign_export.h"
#include "crow_win.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT RequestAuthenticationBase
	{
	public:
		virtual crow::response AuthenticateRequest(const crow::request& req);

	protected:
		RequestAuthenticationBase(bool disableAuthentication);

		// pure virtual interface
		RequestAuthenticationBase() = default;

		bool m_disableAuthentication = false;

	};
}
