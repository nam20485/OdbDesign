#include "fastmove.h"

using namespace std;
using namespace std::filesystem;

namespace Utils
{
	error_code move(const path& source, const path& dest, bool overwriteExisting)
	{
		error_code ec;

		auto options = copy_options::none;
		if (overwriteExisting)
		{
			options |= copy_options::overwrite_existing;
		}
		// TODO: handle recursive copy for use on directories
		//options |= copy_options::recursive;

		if (copy_file(source, dest, options, ec))
		{
			remove(source, ec);
		}

		return ec;
	}

	error_code fastmove(const path& source, const path& dest, bool overwriteExisting)
	{
		error_code ec;

		try
		{
			rename(source, dest);
		}
		catch (filesystem_error& fe)
		{
			// can't rename across devices- try standard copy and remove
			if (fe.code() == std::errc::cross_device_link)
			{
				ec = move(source, dest, overwriteExisting);
			}
			else
			{
				throw fe;
			}
		}

		return ec;
	}
}