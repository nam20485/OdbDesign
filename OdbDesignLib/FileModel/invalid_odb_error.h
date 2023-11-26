#pragma once
#include <exception>
#include <filesystem>
#include <string>

namespace Odb::Lib::FileModel
{
	class invalid_odb_error : public std::runtime_error
	{
	public:
		invalid_odb_error(const char* message)
			: runtime_error(message)
		{
		}

		invalid_odb_error(const std::string& message)
			: invalid_odb_error(message.c_str())
		{			
		}
	};
}