#include "libarchive_extract.h"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <filesystem>
#include "Logger.h"
#include "PathSafety.h"
#include <cstdio>
#include "FileReader.h"
#include <sys/stat.h>
#include <iosfwd>


// from: https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor

using namespace std::filesystem;

namespace Utils
{
    namespace
    {
        const size_t BYTES_PER_KB = 1024UL;
        const size_t READ_OPEN_BLOCK_SIZE = BYTES_PER_KB * 10UL;

        // Walk the entry's relative path under destDir one component at a
        // time, refusing to traverse a symlink at any level (a pre-planted
        // symlink inside destDir would otherwise redirect directory creation
        // and ofstream writes outside the extraction root - the guarantee the
        // old ARCHIVE_EXTRACT_SECURE_SYMLINKS flag provided). create_directory
        // (singular) is used deliberately: it fails rather than follows when
        // the name already exists, and a symlink check after a failed create
        // closes the create-then-swap window as far as the filesystem allows.
        bool securely_create_directories(const std::filesystem::path& base,
                                         const std::filesystem::path& relativePath,
                                         std::string& errorOut)
        {
            // the caller-supplied destination root may not exist yet; it is
            // trusted (operator-configured, canonicalized by ArchiveExtractor
            // before it reaches us), so create it recursively
            std::error_code baseEc;
            if (!exists(base, baseEc) || baseEc)
            {
                create_directories(base, baseEc);
                if (baseEc)
                {
                    errorOut = "failed creating destination root [" + base.string() + "]: " + baseEc.message();
                    return false;
                }
            }

            std::filesystem::path current = base;
            for (const auto& component : relativePath)
            {
                const auto name = component.string();
                // a trailing separator ("step/") iterates as one real
                // component plus an empty artifact - skip the artifact; the
                // entry-name validation has already applied the real policy
                if (name.empty())
                {
                    continue;
                }
                if (name == "." || name == "..")
                {
                    errorOut = "unsafe component [" + name + "]";
                    return false;
                }

                current /= component;

                // Derive existence AND type from symlink_status (lstat - never
                // follows links). exists()/is_directory() follow symlinks, so
                // a link swapped in after a separate is_symlink probe would
                // silently redirect the rest of the walk outside the root.
                // One non-following status call closes that traversal window;
                // the create-then-swap case stays covered by the post-create
                // re-check below. (A swap between this status call and a later
                // syscall is inherent to path-based APIs; the per-component
                // refusal and the entry-name policy bound what it can reach.)
                std::error_code ec;
                const auto status = symlink_status(current, ec);
                if (ec && ec != std::errc::no_such_file_or_directory)
                {
                    errorOut = "failed stat-ing [" + current.string() + "]: " + ec.message();
                    return false;
                }
                if (status.type() == std::filesystem::file_type::not_found)
                {
                    create_directory(current, ec);
                    if (ec)
                    {
                        std::error_code swapEc;
                        if (is_symlink(current, swapEc))
                        {
                            errorOut = "refusing symlink component [" + current.string() + "]";
                            return false;
                        }
                        errorOut = "failed creating [" + current.string() + "]: " + ec.message();
                        return false;
                    }
                }
                else if (status.type() != std::filesystem::file_type::directory)
                {
                    // symlink, regular file, device... anything that is not a
                    // real directory component is refused
                    errorOut = "path component is not a directory [" + current.string() + "]";
                    return false;
                }
            }
            return true;
        }
    }

    bool extract(const char* filename, const char* destDir)
    {
        // Security-audit F003: this extractor previously handed every entry to
        // archive_write_disk with PERM/ACL/FFLAGS restoration enabled, so a
        // crafted archive chose its own file modes, and ARCHIVE_EXTRACT_SECURE_
        // NOABSOLUTEPATHS could not simply be turned on: the write path
        // PREPENDS destDir to each entry and writes the joined ABSOLUTE
        // pathname, which that flag rejects outright (why it was commented
        // out). Instead of patching flags onto a write path we do not
        // control, extraction now takes write ownership itself: every entry
        // name is validated BEFORE any join, regular files are written
        // through ofstream (umask permissions - archive modes are never
        // applied), directories are created with default permissions, and
        // every other entry type (symlink, hardlink, device, fifo) is
        // refused. ODB++ design archives carry only files and directories,
        // so nothing legitimate is lost.
        struct archive* a;
        struct archive_entry* entry;
        int r;

        a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        r = archive_read_open_filename(a, filename, READ_OPEN_BLOCK_SIZE);
        if (r != ARCHIVE_OK)
        {
            auto errNo = archive_errno(a);
            fprintf(stderr, "%s (%d)\n", archive_error_string(a), errNo);
            archive_read_free(a);
            return false;
        }

        // Windows long-path safety (security-audit F003 follow-up): libarchive's
        // write path silently applied \\?\ prefixes on Windows
        // (__la_win_permissive_name), which lifted the 248-character
        // CreateDirectoryW directory limit (and the 260-character file limit).
        // Taking write ownership with std::filesystem + ofstream lost that:
        // extraction destinations nest the archive stem twice by design
        // (dest/<stem>/<stem>/... - findRootDir descends one level), and deep
        // legitimate design trees crossed 248 chars, failing extraction with
        // ERROR_FILENAME_EXCED_RANGE ("The filename or extension is too
        // long"). Prefix the absolute destination with the literal wide-path
        // form so every create/open below can address the full tree. The
        // entry names joined onto it are validated relative paths (no . or ..
        // components), and ArchiveExtractor canonicalizes the destination
        // before it reaches us. No-op on non-Windows.
        std::filesystem::path destinationDir(destDir);
#if defined(_WIN32)
        {
            std::error_code absoluteError;
            const auto absoluteDestination = std::filesystem::absolute(destinationDir, absoluteError);
            if (!absoluteError)
            {
                auto wide = absoluteDestination.wstring();
                if (wide.rfind(L"\\\\?\\", 0) == 0)
                {
                    destinationDir = std::filesystem::path(std::move(wide));
                }
                else if (wide.rfind(L"\\\\", 0) == 0)
                {
                    wide.insert(2, L"?\\UNC\\");
                    destinationDir = std::filesystem::path(std::move(wide));
                }
                else
                {
                    destinationDir = std::filesystem::path(L"\\\\?\\" + wide);
                }
                // the \\?\ form is literal: normalize separators on the ROOT
                // too. MSVC's absolute() normalizes via GetFullPathNameW, but
                // MinGW/libstdc++ preserves input separators for not-yet-
                // existing components, so a "C:/designs"-style root would
                // otherwise yield \\?\C:/designs and fail every create/open.
                destinationDir.make_preferred();
            }
            else
            {
                // proceeding unprefixed silently disables the long-path fix;
                // make that degradation visible
                logwarn("extract: absolute() failed for destination [" + destinationDir.string()
                    + "]: " + absoluteError.message() + " - proceeding without long-path prefix");
            }
        }
#endif

        for (;;) {
            r = archive_read_next_header(a, &entry);
            if (r == ARCHIVE_EOF)
                break;
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(a));
            if (r < ARCHIVE_WARN)
            {
                archive_read_free(a);
                return false;
            }

            // Validate the entry name BEFORE joining. std::filesystem
            // "base / name" discards the base entirely for a rooted name, and
            // a ".." component climbs out - reject both (and control
            // characters, drive forms, and backslash traversal). A hostile
            // entry fails the whole extraction: silently skipping an entry
            // would yield a design that parses wrong.
            std::string entryPathname = archive_entry_pathname(entry);
            if (entryPathname.empty() || !is_safe_relative_entry_path(entryPathname))
            {
                logerror("Refusing unsafe archive entry name: [" + entryPathname + "]");
                archive_read_free(a);
                return false;
            }
            const std::filesystem::path relativeEntryPath(entryPathname);
            // A \\?\-prefixed destination is interpreted literally: the
            // forward slashes carried by tar entry names must be normalized
            // to native separators before the join, or every create/open
            // below fails with invalid path syntax on Windows.
            std::filesystem::path entryDestinationPath = destinationDir / relativeEntryPath;
            entryDestinationPath.make_preferred();

            logdebug("Extracting, destination dir: [" + std::string(destDir) + "]");
            logdebug("Extracting, entry path name: [" + entryPathname + "]");
            logdebug("Extracting, destination path name: [" + entryDestinationPath.string() + "]");

            // tar hardlink entries ('1') surface as AE_IFREG with the target
            // in archive_entry_hardlink() and size 0 - check the link targets
            // explicitly so a hardlink cannot create an alias/truncation
            // primitive inside the root
            if (archive_entry_hardlink(entry) != nullptr || archive_entry_symlink(entry) != nullptr)
            {
                logerror("Refusing link archive entry: [" + entryPathname + "]");
                archive_read_free(a);
                return false;
            }
            const auto fileType = archive_entry_filetype(entry);
            if (fileType == AE_IFDIR)
            {
                std::string walkError;
                if (!securely_create_directories(destinationDir, relativeEntryPath, walkError))
                {
                    logerror("Failed creating extraction directory for [" + entryPathname + "]: " + walkError);
                    archive_read_free(a);
                    return false;
                }
                continue;
            }

            if (fileType != AE_IFREG)
            {
                // symlinks/devices/fifos have no place in a design archive
                // and are write primitives in every other context
                logerror("Refusing non-regular archive entry: [" + entryPathname + "]");
                archive_read_free(a);
                return false;
            }

            {
                std::string walkError;
                if (!securely_create_directories(destinationDir, relativeEntryPath.parent_path(), walkError))
                {
                    logerror("Failed creating extraction parent directory for [" + entryPathname + "]: " + walkError);
                    archive_read_free(a);
                    return false;
                }
            }

            // a pre-planted symlink AT the final name would make ofstream
            // clobber its target outside the root
            std::error_code targetSymlinkEc;
            if (is_symlink(entryDestinationPath, targetSymlinkEc))
            {
                logerror("Refusing symlink extraction target [" + entryDestinationPath.string() + "]");
                archive_read_free(a);
                return false;
            }

            std::ofstream outputStream(entryDestinationPath, std::ios::binary | std::ios::trunc);
            if (!outputStream)
            {
                logerror("Failed opening extraction file [" + entryDestinationPath.string() + "]");
                archive_read_free(a);
                return false;
            }

            // stream the entry payload block by block; read_data_block
            // reports EOF at the end of THIS entry's data. Extents must be
            // contiguous: any gap or regression fails the extraction (see
            // the check below).
            la_int64_t expectedOffset = 0;
            for (;;)
            {
                const void* buff = nullptr;
                size_t size = 0;
                la_int64_t offset = 0;

                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                {
                    break;
                }
                if (r < ARCHIVE_OK)
                {
                    fprintf(stderr, "%s\n", archive_error_string(a));
                    archive_read_free(a);
                    return false;
                }

                // Design archives are contiguous by construction (CAD tools
                // do not emit sparse tars): ANY non-contiguous extent - a
                // declared sparse hole (offset > expected, which zero-filling
                // would materialize as attacker-declared unbounded zeros: a
                // disk-exhaustion DoS) or a backwards/overlapping map
                // (offset < expected, which silently shifts/duplicates
                // content) - is malformed or hostile, so fail closed.
                if (offset != expectedOffset)
                {
                    logerror("Non-contiguous data extents for archive entry [" + entryPathname + "] (expected offset "
                        + std::to_string(expectedOffset) + ", got " + std::to_string(offset) + ")");
                    archive_read_free(a);
                    return false;
                }

                outputStream.write(static_cast<const char*>(buff), static_cast<std::streamsize>(size));
                if (!outputStream)
                {
                    logerror("Failed writing extraction file [" + entryDestinationPath.string() + "]");
                    archive_read_free(a);
                    return false;
                }
                expectedOffset = offset + static_cast<la_int64_t>(size);
            }
            // the entry must be COMPLETE: an entry whose declared size
            // exceeds its actual data (e.g. a sparse record with a trailing
            // hole) would otherwise write a short file and report success -
            // inconsistent with the fail-closed extents posture above
            if (expectedOffset != archive_entry_size(entry))
            {
                logerror("Incomplete archive entry [" + entryPathname + "] (declared "
                    + std::to_string(archive_entry_size(entry)) + " bytes, got "
                    + std::to_string(expectedOffset) + ")");
                archive_read_free(a);
                return false;
            }
            // the final flush happens at close - check it so ENOSPC/EIO is
            // not reported as a successful extraction
            outputStream.close();
            if (!outputStream)
            {
                logerror("Failed flushing extraction file [" + entryDestinationPath.string() + "]");
                archive_read_free(a);
                return false;
            }
        }

        archive_read_close(a);
        archive_read_free(a);

        return true;
    }


    constexpr inline static const char* TGZ_EXTENSION = ".tgz";
    constexpr static const int READ_BUFF_SIZE = 1024;    

    bool compress_dir(const char* srcDir, const char* destDir, const char* archiveName, std::string& fileOut, CompressionType type /* = CompressionType::TarGzip*/)
    {
        path destArchivePath = path(destDir) / archiveName;
        switch (type)
        {
        case CompressionType::TarGzip:
            destArchivePath.replace_extension(TGZ_EXTENSION);
            break;
        }

        struct archive* a = archive_write_new();
        //if (archive_write_set_format_v7tar(a) != ARCHIVE_OK) return false;
        if (archive_write_set_format_ustar(a) != ARCHIVE_OK) return false;
        //if (archive_write_set_format_gnutar(a) != ARCHIVE_OK) return false;
        if (archive_write_add_filter_gzip(a) != ARCHIVE_OK) return false;

        if (archive_write_open_filename(a, destArchivePath.string().c_str()) != ARCHIVE_OK) return false;

        //// add top-level directory to the archive
        auto root_entry = archive_entry_new();
        //auto rootDir = path(archiveName);
        auto rootDir = path(srcDir).filename();
        archive_entry_set_pathname(root_entry, rootDir.string().c_str());
        archive_entry_set_filetype(root_entry, AE_IFDIR);
        if (archive_write_header(a, root_entry) != ARCHIVE_OK) return false;
        archive_entry_free(root_entry);

        struct stat st;
        
        // add files to the archive               
        for (const auto& it : recursive_directory_iterator(srcDir))
		{
            if (it.is_directory())
            {                                              
                auto relativePath = relative(it.path(), srcDir);
                auto dir_entry = archive_entry_new();
                // generic_string(): tar entry names must use '/' on every
                // host OS - a Windows-written archive with '\' names extracts
                // as literal-backslash filenames on POSIX (and ustar's
                // prefix/name split only works at '/' boundaries).
                archive_entry_set_pathname(dir_entry, (rootDir / relativePath).generic_string().c_str());
                archive_entry_set_filetype(dir_entry, AE_IFDIR);
                stat(it.path().string().c_str(), &st);
                archive_entry_copy_stat(dir_entry, &st);
                if (archive_write_header(a, dir_entry) != ARCHIVE_OK) return false;
                archive_write_finish_entry(a);
                archive_entry_free(dir_entry);
            }
			else if (it.is_regular_file())
			{
                // write header				
                auto relativePath = relative(it.path(), srcDir);
                auto file_entry = archive_entry_new();
                archive_entry_set_pathname(file_entry, (rootDir / relativePath).generic_string().c_str());
                stat(it.path().string().c_str(), &st);
                archive_entry_set_filetype(file_entry, AE_IFREG);
                archive_entry_set_size(file_entry, st.st_size);  // Note 2
                //archive_entry_set_mtime(file_entry, st.st_mtime, 0);
                //archive_entry_set_perm(file_entry, st.st_mode?);                
                archive_entry_copy_stat(file_entry, &st);
                if (archive_write_header(a, file_entry) != ARCHIVE_OK) return false;
                
                std::ifstream ifs(it.path(), std::ios::in | std::ios::binary);
                if (!ifs.is_open()) return false;
                char szBuffer[READ_BUFF_SIZE]{ 0 };
                while (true)
                {
                    ifs.read(szBuffer, sizeof(szBuffer));
                    auto readin = ifs.gcount();
                    if (readin < 1)
                    {
                        break;
                    }
                    if (archive_write_data(a, szBuffer, readin) != readin) return false;
                }
                ifs.close();

                // read file and write to archive
                //FileReader fr(it.path());
                //auto read = fr.Read();
                //if (read > 0)
                //{
                //    auto& buffer = fr.GetBuffer();
                //    if (archive_write_data(a, buffer.data(), read) != read) return false;
                //}                               
                archive_write_finish_entry(a);
                archive_entry_free(file_entry);
			}
		}

        // clean up        
        archive_write_close(a);
        archive_write_free(a);

        fileOut = destArchivePath.string();
        return true;
    }     
}