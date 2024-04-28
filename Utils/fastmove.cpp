#include "fastmove.h"
#include <system_error>

using namespace std;
using namespace std::filesystem;

namespace Utils
{
	void move_file(const path& source, const path& dest, bool overwriteExisting, std::error_code& ec)
	{
		auto options = copy_options::none;
		if (overwriteExisting)
		{
			options |= copy_options::overwrite_existing;
		}		

		if (copy_file(source, dest, options, ec) && 
			ec.value() == 0)
		{
			remove(source, ec);
		}
	}

	void fastmove_file(const path& source, const path& dest, bool overwriteExisting, std::error_code& ec)
	{
		//try
		{
			rename(source, dest, ec);
			if (ec.value() == static_cast<int>(std::errc::cross_device_link))
			{
				move_file(source, dest, overwriteExisting, ec);
			}
		}
		//catch (filesystem_error& fe)
		//{
		//	// can't rename across devices- try standard copy and remove
		//	if (fe.code() == std::errc::cross_device_link)
		//	{
		//		ec = move_file(source, dest, overwriteExisting);
		//	}
		//	else
		//	{
		//		throw fe;
		//	}
		//}

		//return ec;
	}
}