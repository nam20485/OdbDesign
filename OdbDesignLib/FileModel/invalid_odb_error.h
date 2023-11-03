#pragma once
#include <exception>
#include <filesystem>
#include <string>

namespace Odb::Lib::FileModel
{
	class invalid_odb_error : public std::exception
	{
	public:
		invalid_odb_error(char const* const message)
			: exception(message)
		{
		}

		invalid_odb_error(const std::string& message)
			: invalid_odb_error(message.c_str())
		{			
		}
	};
}