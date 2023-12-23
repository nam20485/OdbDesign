#pragma once

#include "IOdbApp.h"
#include "../odbdesign_export.h"
#include "IHttpServer.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IOdbServerApp : public virtual IOdbApp
	{
	public:
		virtual ~IOdbServerApp() {}

		//virtual crow::SimpleApp& crow_app() = 0;
		virtual IHttpServer& http_server() = 0;

	protected:
		IOdbServerApp() = default;

	};
}
