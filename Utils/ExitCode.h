#pragma once


namespace Utils
{
	enum class ExitCode
	{
		Success = 0,
		FailedInit = 1,		
		FailedInitSsl = 2,
		FailedInitSslDirDoesNotExist = 3,
		FailedInitLoadDesign = 4,
		PreServerRunFailed = 5,
		PostServerRunFailed = 6,
		UnknownError = 7
	};
}
