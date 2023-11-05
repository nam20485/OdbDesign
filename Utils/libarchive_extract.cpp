#include "libarchive_extract.h"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <filesystem>


// from: https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor

namespace Utils
{
    int copy_data(struct archive* ar, struct archive* aw);

    const bool SECURE_EXTRACTION = true;

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
            flags |= ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;
        }

        const size_t READ_OPEN_BLOCK_SIZE = 1024 * 10;

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
            auto entryPathname = archive_entry_pathname(entry);
            std::filesystem::path entryDestinationPathname(destDir);
            entryDestinationPathname /= entryPathname;
            archive_entry_set_pathname(entry, entryDestinationPathname.string().c_str());

            r = archive_write_header(ext, entry);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(ext));
            else if (archive_entry_size(entry) > 0) {
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

        for (;;) {
            r = archive_read_data_block(ar, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
                return (ARCHIVE_OK);
            if (r < ARCHIVE_OK)
                return (r);
            written = archive_write_data_block(aw, buff, size, offset);
            if (written < ARCHIVE_OK) {
                fprintf(stderr, "%s\n", archive_error_string(aw));
                return (r);
            }
        }
    }
}