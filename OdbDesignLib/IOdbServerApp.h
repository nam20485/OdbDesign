#pragma once

#include "IOdbApp.h"
#include "crow_win.h"
#include "odbdesign_export.h"

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT IOdbServerApp : public virtual IOdbApp
	{
	public:
		virtual ~IOdbServerApp() {}

		virtual crow::SimpleApp& crow_app() = 0;

	protected:
		IOdbServerApp() = default;

	};
}
