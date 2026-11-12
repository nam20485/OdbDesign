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
        // Security-audit F002: POSIX rename() silently replaces an existing
        // destination, which made the overwriteExisting=false contract a no-op
        // - callers asking for no-overwrite (design upload) were overwriting
        // stored files anyway. Honor the flag: refuse when the destination
        // exists unless overwrite was explicitly requested. Note the check is
        // not atomic with rename(): two concurrent no-overwrite moves of the
        // same name race to last-writer-wins (still contained to the caller's
        // destination directory, so this is a correctness nicety, not a
        // containment hole).
        ec.clear();
        if (!overwriteExisting)
        {
            std::error_code existsEc;
            const bool destExists = exists(dest, existsEc);
            if (existsEc)
            {
                ec = existsEc;
                return;
            }
            if (destExists)
            {
                ec = std::make_error_code(std::errc::file_exists);
                return;
            }
        }

        rename(source, dest, ec);
        if (ec.value() == static_cast<int>(std::errc::cross_device_link))
        {
            move_file(source, dest, overwriteExisting, ec);
        }
    }
}