#include "fastcopy.h"

using namespace std;
using namespace std::filesystem;

namespace Utils
{
	error_code copy(const path& source, const path& dest, bool overwriteExisting)
	{
		error_code ec;

		auto options = copy_options::none;
		if (overwriteExisting)
		{
			options = copy_options::overwrite_existing;
		}

		if (copy_file(source, dest, options, ec))
		{
			remove(source, ec);
		}

		return ec;
	}

	error_code copy(const string& source, const string& dest, bool overwriteExisting)
	{
		return copy(path(source), path(dest), overwriteExisting);
	}

	error_code fastcopy(const string& source, const string& dest, bool overwriteExisting)
	{
		return fastcopy(path(source), path(dest), overwriteExisting);
	}

	error_code fastcopy(const path& source, const path& dest, bool overwriteExisting)
	{
		error_code ec;

		try
		{
			rename(source, dest, ec);
		}
		catch (filesystem_error& fe)
		{
			// can't rename across devices- try standard copy and remove
			if (fe.code() == std::errc::cross_device_link)
			{
				ec = copy(source, dest, overwriteExisting);
			}
			else
			{
				throw fe;
			}
		}

		return ec;
	}
}