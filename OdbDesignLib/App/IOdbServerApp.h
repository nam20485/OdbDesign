#pragma once

#include "IOdbApp.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IOdbServerApp : public virtual IOdbApp
	{
	public:
		virtual ~IOdbServerApp() {}

		virtual CrowApp& crow_app() = 0;

	protected:
		IOdbServerApp() = default;

	};
}
