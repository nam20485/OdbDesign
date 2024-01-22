#pragma once

#include "IOdbApp.h"
#include "../odbdesign_export.h"
#include "IRequestAuthentication.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IOdbServerApp : public virtual IOdbApp
	{
	public:
		virtual ~IOdbServerApp() {}

		virtual CrowApp& crow_app() = 0;
		virtual IRequestAuthentication& request_auth() = 0;
		virtual void request_auth(std::unique_ptr<IRequestAuthentication> requestAuthentication) = 0;

	protected:
		IOdbServerApp() = default;

	};
}
