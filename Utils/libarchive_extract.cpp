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
    const size_t BYTES_PER_KB = 1024UL;
    const size_t READ_OPEN_BLOCK_SIZE = BYTES_PER_KB * 10UL;

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

        const std::filesystem::path destinationDir(destDir);

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
            const auto entryDestinationPath = destinationDir / entryPathname;

            logdebug("Extracting, destination dir: [" + std::string(destDir) + "]");
            logdebug("Extracting, entry path name: [" + entryPathname + "]");
            logdebug("Extracting, destination path name: [" + entryDestinationPath.string() + "]");

            const auto fileType = archive_entry_filetype(entry);
            if (fileType == AE_IFDIR)
            {
                std::error_code ec;
                create_directories(entryDestinationPath, ec);
                if (ec)
                {
                    logerror("Failed creating extraction directory [" + entryDestinationPath.string() + "]: " + ec.message());
                    archive_read_free(a);
                    return false;
                }
                continue;
            }

            if (fileType != AE_IFREG)
            {
                // symlinks/hardlinks/devices/fifos have no place in a design
                // archive and are write primitives in every other context
                logerror("Refusing non-regular archive entry: [" + entryPathname + "]");
                archive_read_free(a);
                return false;
            }

            std::error_code ec;
            if (entryDestinationPath.has_parent_path())
            {
                create_directories(entryDestinationPath.parent_path(), ec);
                if (ec)
                {
                    logerror("Failed creating extraction parent directory [" + entryDestinationPath.parent_path().string() + "]: " + ec.message());
                    archive_read_free(a);
                    return false;
                }
            }

            std::ofstream outputStream(entryDestinationPath, std::ios::binary | std::ios::trunc);
            if (!outputStream)
            {
                logerror("Failed opening extraction file [" + entryDestinationPath.string() + "]");
                archive_read_free(a);
                return false;
            }

            // stream the entry payload block by block; read_data_block
            // reports EOF at the end of THIS entry's data
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

                outputStream.write(static_cast<const char*>(buff), static_cast<std::streamsize>(size));
                if (!outputStream)
                {
                    logerror("Failed writing extraction file [" + entryDestinationPath.string() + "]");
                    archive_read_free(a);
                    return false;
                }
            }
            outputStream.close();
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
                archive_entry_set_pathname(dir_entry, (rootDir / relativePath).string().c_str());
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
                archive_entry_set_pathname(file_entry, (rootDir / relativePath).string().c_str());
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