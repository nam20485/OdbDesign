#pragma once

#include "../odbdesign_export.h"
#include "crow_win.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IRequestAuthentication
	{
	public:
		virtual crow::response AuthenticateRequest(const crow::request& req) = 0;

	protected:
		// pure virtual interface
		IRequestAuthentication() = default;

	};
}
