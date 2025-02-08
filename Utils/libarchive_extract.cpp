#include "libarchive_extract.h"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <filesystem>
#include "Logger.h"
#include <cstdio>
#include "FileReader.h"
#include <sys/stat.h>
#include <iosfwd>


// from: https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor

using namespace std::filesystem;

namespace Utils
{
    int copy_data(struct archive* ar, struct archive* aw);

    const bool SECURE_EXTRACTION = true;
    const size_t BYTES_PER_KB = 1024UL;
    const size_t READ_OPEN_BLOCK_SIZE = BYTES_PER_KB * 10UL;

    bool extract(const char* filename, const char* destDir)
    {
        struct archive* a;
        struct archive* ext;
        struct archive_entry* entry;
        int flags;
        int r;

        /* Select which attributes we want to restore. */
        flags = ARCHIVE_EXTRACT_TIME;
        flags |= ARCHIVE_EXTRACT_PERM;
        flags |= ARCHIVE_EXTRACT_ACL;
        flags |= ARCHIVE_EXTRACT_FFLAGS;

        if (SECURE_EXTRACTION)
        {
            flags |= ARCHIVE_EXTRACT_SECURE_SYMLINKS;
            flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;
            //flags |= ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;
        }        

        a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        //archive_read_support_format_tar(a);
        //archive_read_support_filter_gzip(a);
        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, flags);
        archive_write_disk_set_standard_lookup(ext);
        r = archive_read_open_filename(a, filename, READ_OPEN_BLOCK_SIZE);
        if (r != ARCHIVE_OK)
        {
            auto errNo = archive_errno(a);
            fprintf(stderr, "%s (%d)\n", archive_error_string(a), errNo);
            return false;
        }

        for (;;) {
            r = archive_read_next_header(a, &entry);
            if (r == ARCHIVE_EOF)
                break;
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(a));
            if (r < ARCHIVE_WARN)
            {
                return false;
            }

            // prepend destPath to beginning of entry to extract it in the destination path
            std::string entryPathname = archive_entry_pathname(entry);
            std::filesystem::path entryDestinationPathname(destDir);
            entryDestinationPathname /= entryPathname;  
            //auto relativeEntryDestinationPathname = relative(entryDestinationPathname, current_path());

            logdebug("Extracting, destination dir: [" + std::string(destDir) + "]");
            logdebug("Extracting, entry path name: [" + entryPathname + "]");
            logdebug("Extracting, destination path name: [" + entryDestinationPathname.string() + "]");
            //loginfo("Extracting, relative destination path name: [" + relativeEntryDestinationPathname.string() + "]");

            archive_entry_set_pathname(entry, entryDestinationPathname.string().c_str());

            r = archive_write_header(ext, entry);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "archive_write_header failed: %s\n", archive_error_string(ext));
            else if (archive_entry_size(entry) > 0)
            {
                r = copy_data(a, ext);
                if (r < ARCHIVE_OK)
                    fprintf(stderr, "%s\n", archive_error_string(ext));
                if (r < ARCHIVE_WARN)
                {
                    return false;
                }
            }
            r = archive_write_finish_entry(ext);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(ext));
            if (r < ARCHIVE_WARN)
            {
                return false;
            }
        }
        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);

        return true;
    }  

    int copy_data(struct archive* ar, struct archive* aw)
    {
        int r;
        const void* buff;
        size_t size;
        la_int64_t offset;
        la_ssize_t written;

        for (;;)
        {
            r = archive_read_data_block(ar, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
            {
                return (ARCHIVE_OK);
            }
            else if (r < ARCHIVE_OK)
            {
                return (r);
            }

            written = archive_write_data_block(aw, buff, size, offset);
            if (written < ARCHIVE_OK)
            {
                fprintf(stderr, "%s\n", archive_error_string(aw));
                return (r);
            }
        }
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