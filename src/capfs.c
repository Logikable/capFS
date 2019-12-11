/*
**  ----- BEGIN LICENSE BLOCK -----
**  CapFS: (GDP Data)Capsule File System
**  From the Ubiquitous Swarm Lab, 490 Cory Hall, U.C. Berkeley.
**
**  Copyright (c) 2019, Regents of the University of California.
**  Copyright (c) 2019, Sean Luchen, Jie Chen, and SangBin Cho
**  All rights reserved.
**
**  Permission is hereby granted, without written agreement and without
**  license or royalty fees, to use, copy, modify, and distribute this
**  software and its documentation for any purpose, provided that the above
**  copyright notice and the following two paragraphs appear in all copies
**  of this software.
**
**  IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
**  SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
**  PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
**  EVEN IF REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**  REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
**  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
**  FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION,
**  IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO
**  OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
**  OR MODIFICATIONS.
**  ----- END LICENSE BLOCK -----
*/

#include "capfs.h"

#include <string.h>
#include <sysexits.h>

#include <ep/ep.h>
#include <gdp/gdp.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 30
#endif

#include <fuse.h>

#include "capfs_file.h"
#include "capfs_dir.h"
#include "capfs_util.h"

static int
capfs_access(const char *path, int mode) {
    return 0;
}

static int
capfs_chmod(const char *filepath, mode_t mode) {
    return 0;
}

static int
capfs_chown(const char *filepath, uid_t uid, gid_t gid) {
    return 0;
}

static int
capfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;

    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    if (path[strlen(path) - 1] == '/') {
        return -EISDIR;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Check file does not exist
    if (capfs_dir_has_child(dir, file_name, false)) {
        goto fail1;
    }

    // Get a new file handler
    fh_entry_t *fh;
    estat = fh_new(&fh);
    EP_STAT_CHECK(estat, goto fail2);
    fi->fh = fh->fh;

    // Create the file in the directory
    capfs_file_t *file;
    estat = capfs_dir_make_file(dir, file_name, &file);
    EP_STAT_CHECK(estat, goto fail2);
    // Store data in file handler
    fh->is_dir = false;
    fh->file = file;
    fh->ref++;

    // Cleanup
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail2:
    fh_free(fh->fh);
fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_getattr(const char *path, struct stat *st) {
    EP_STAT estat;
    memset(st, 0, sizeof(struct stat));

    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    capfs_file_t *file;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Check if it's a directory first
    capfs_dir_t *child;
    estat = capfs_dir_opendir(dir, name, &child);
    EP_STAT_CHECK(estat, goto not_dir);

    // Valid directory
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 2;

    // Cleanup
    capfs_dir_closedir(child);
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

not_dir:
    // Check if it's a file
    estat = capfs_dir_open_file(dir, name, &file);
    EP_STAT_CHECK(estat, goto fail1);

    // Valid file
    st->st_mode = S_IFREG | 0777;
    st->st_nlink = 1;
    estat = capfs_file_get_length(file, (size_t *) &(st->st_size));
    EP_STAT_CHECK(estat, goto fail2);

    // Cleanup
    capfs_file_close(file);
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail2:
    capfs_file_close(file);
fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_mkdir(const char *path, mode_t mode) {
    (void) mode;

    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *dir_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Child does not exist
    if (capfs_dir_has_child(dir, dir_name, true)) {
        goto fail1;
    }

    // Create the child in the directory
    capfs_dir_t *child;
    estat = capfs_dir_mkdir(dir, dir_name, &child);
    EP_STAT_CHECK(estat, goto fail1);

    // Cleanup
    capfs_dir_closedir(child);
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_open(const char *path, struct fuse_file_info *fi) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    if (path[strlen(path) - 1] == '/') {
        return -EISDIR;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Open file
    capfs_file_t *file;
    estat = capfs_dir_open_file(dir, file_name, &file);
    EP_STAT_CHECK(estat, goto fail1);

    fh_entry_t *fh;
    estat = fh_get_by_gob(file->gob, &fh);
    // File already exists
    if (EP_STAT_ISOK(estat)) {
        fh->ref++;
        capfs_file_close(file);
    } else {    // doesn't exist
        // Get a new file handler
        estat = fh_new(&fh);
        EP_STAT_CHECK(estat, goto fail1);
        fi->fh = fh->fh;

        // Store data in file handler
        fh->is_dir = false;
        fh->file = file;
        fh->ref++;
    }

    // Cleanup
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_opendir(const char *path, struct fuse_file_info *fi) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Open child
    capfs_dir_t *child;
    estat = capfs_dir_opendir(dir, file_name, &child);
    EP_STAT_CHECK(estat, goto fail1);

    fh_entry_t *fh;
    estat = fh_get_by_gob(child->file->gob, &fh);
    // File already exists
    if (EP_STAT_ISOK(estat)) {
        fh->ref++;
        capfs_dir_closedir(child);
    } else {    // doesn't exist
        // Get a new file handler
        estat = fh_new(&fh);
        EP_STAT_CHECK(estat, goto fail1);
        fi->fh = fh->fh;

        // Store data in file handler
        fh->is_dir = true;
        fh->dir = child;
        fh->ref++;
    }

    // Cleanup
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi) {
    (void) path;
    EP_STAT estat;

    fh_entry_t *fh;
    estat = fh_get(fi->fh, &fh);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity checks
    if (!fh->valid || fh->is_dir) {
        goto fail0;
    }

    estat = capfs_file_read(fh->file, buf, size, offset);
    EP_STAT_CHECK(estat, goto fail0);
    return 0;

fail0:
    return -ENOENT;
}

static int
capfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi) {
    (void) path;
    (void) offset;
    EP_STAT estat;

    fh_entry_t *fh;
    estat = fh_get(fi->fh, &fh);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity checks
    if (!fh->valid || !fh->is_dir) {
        goto fail0;
    }

    // Read in names
    capfs_dir_table_t table;
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    estat = capfs_dir_readdir(fh->dir, &table, names, NULL);
    EP_STAT_CHECK(estat, goto fail0);

    // Push names through filler
    for (size_t i = 0; i < table.length; i++) {
        filler(buf, names[i], NULL, 0);
    }
    return 0;

fail0:
    return -ENOENT;
}

// Equivalent to file_close
static int
capfs_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    EP_STAT estat;

    fh_entry_t *fh;
    estat = fh_get(fi->fh, &fh);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity checks
    if (!fh->valid || fh->is_dir) {
        goto fail0;
    }

    // Only close and free if unreferenced
    fh->ref--;
    if (fh->ref == 0) {
        capfs_file_close(fh->file);
        fh_free(fh->fh);
    }
    return 0;

fail0:
    return -ENOENT;
}

static int
capfs_releasedir(const char *path, struct fuse_file_info *fi) {
    (void) path;
    EP_STAT estat;

    fh_entry_t *fh;
    estat = fh_get(fi->fh, &fh);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity checks
    if (!fh->valid || !fh->is_dir) {
        goto fail0;
    }

    // Only close and free if unreferenced
    fh->ref--;
    if (fh->ref == 0) {
        capfs_dir_closedir(fh->dir);
        fh_free(fh->fh);
    }
    return 0;

fail0:
    return -ENOENT;
}

static int
capfs_rename(const char *from, const char *to) {
    if (strlen(from) == 0 || strlen(to) == 0) {
        return -ENOENT;
    }
    EP_STAT estat;

    // Open from directory
    char **path_tokens;
    size_t num_tokens = split_path(from, &path_tokens);
    char from_name[FILE_NAME_MAX_LEN + 1];
    strcpy(from_name, path_tokens[num_tokens - 1]);

    capfs_dir_t *from_dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &from_dir);
    EP_STAT_CHECK(estat, goto fail0);
    free_tokens(path_tokens);

    // Open to directory
    num_tokens = split_path(to, &path_tokens);
    char to_name[FILE_NAME_MAX_LEN + 1];
    strcpy(to_name, path_tokens[num_tokens - 1]);

    capfs_dir_t *to_dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &to_dir);
    EP_STAT_CHECK(estat, goto fail1);

    // Rename
    estat = capfs_dir_rename(from_dir, to_dir, from_name, to_name);
    EP_STAT_CHECK(estat, goto fail2);

    // Cleanup
    capfs_dir_closedir(to_dir);
    capfs_dir_closedir(from_dir);
    free_tokens(path_tokens);
    return 0;

fail2:
    capfs_dir_closedir(to_dir);
fail1:
    capfs_dir_closedir(from_dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_rmdir(const char *path) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *dir_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    estat = capfs_dir_rmdir(dir, dir_name);
    EP_STAT_CHECK(estat, goto fail1);

    // Cleanup
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_truncate(const char *path, off_t file_size) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    if (path[strlen(path) - 1] == '/') {
        return -EISDIR;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Open file
    capfs_file_t *file;
    estat = capfs_dir_open_file(dir, file_name, &file);
    EP_STAT_CHECK(estat, goto fail1);

    // Truncate
    estat = capfs_file_truncate(file, file_size);
    EP_STAT_CHECK(estat, goto fail2);

    // Cleanup
    capfs_file_close(file);
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail2:
    capfs_file_close(file);
fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_unlink(const char *path) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    EP_STAT estat;

    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Open directory
    capfs_dir_t *dir;
    estat = capfs_dir_opendir_path(path_tokens, num_tokens, &dir);
    EP_STAT_CHECK(estat, goto fail0);

    estat = capfs_dir_remove_file(dir, file_name);
    EP_STAT_CHECK(estat, goto fail1);

    // Cleanup
    capfs_dir_closedir(dir);
    free_tokens(path_tokens);
    return 0;

fail1:
    capfs_dir_closedir(dir);
fail0:
    free_tokens(path_tokens);
    return -ENOENT;
}

static int
capfs_utimens(const char *path, const struct timespec ts[2]) {
    return 0;
}

static int
capfs_write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi) {
    (void) path;
    EP_STAT estat;

    fh_entry_t *fh;
    estat = fh_get(fi->fh, &fh);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity checks
    if (!fh->valid || fh->is_dir) {
        goto fail0;
    }

    estat = capfs_file_write(fh->file, buf, size, offset);
    EP_STAT_CHECK(estat, goto fail0);
    return 0;

fail0:
    return -ENOENT;
}

static struct fuse_operations capfs_operations = {
    .access = capfs_access,
    .chmod = capfs_chmod,
    .chown = capfs_chown,
    .create = capfs_create,
    .getattr = capfs_getattr,
    .mkdir = capfs_mkdir,
    .open = capfs_open,
    .opendir = capfs_opendir,
    .read = capfs_read,
    .readdir = capfs_readdir,
    .release = capfs_release,
    .releasedir = capfs_releasedir,
    .rename = capfs_rename,
    .rmdir = capfs_rmdir,
    .truncate = capfs_truncate,
    .unlink = capfs_unlink,
    .utimens = capfs_utimens,
    .write = capfs_write,
};

void
init(void) {
    fh_init();

    EP_STAT estat = gdp_init(NULL);
    if (!EP_STAT_ISOK(estat)) {
        exit(EX_UNAVAILABLE);
    }
    // Just in case this is a fresh file system
    capfs_dir_make_root();
}

int
run(int argc, char *argv[]) {
    init();
    return fuse_main(argc, argv, &capfs_operations, NULL);
}
