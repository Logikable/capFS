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
#include "capfs_util.h"

static int
capfs_access(const char *path, int mode) {
    return 0;
}

static int
capfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Error handling
    if (strlen(path) == 0) {
        return -ENOENT;
    }
    if (path[strlen(path) - 1] == '/') {
        return -EISDIR;
    }

    // TODO: check existence
    char **path_tokens;
    size_t num_tokens = split_path(path, &path_tokens);
    char *file_name = path_tokens[num_tokens - 1];

    // Get a new file handler
    fh_entry_t *fh;
    EP_STAT_CHECK(fh_new(path, &fh), goto fail0);
    fi->fh = fh->fh;

    // Create the file in GDP
    capfs_file_t *file;
    EP_STAT_CHECK(capfs_file_create(path, &file), goto fail0);
    // TODO: put file in directory

    // Store GOB
    memcpy(fh->gob, file->gob, sizeof(gdp_name_t));
    return 0;

fail0:
    fh_free(fh->fh);
    return -ENOENT;
}

static int
capfs_getattr(const char *path, struct stat *st) {
    memset(st, 0, sizeof(struct stat));

    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    // For valid directories:
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    // For valid files:
    if (true) {
        st->st_mode = S_IFREG | 0777;
        st->st_nlink = 1;
        st->st_size = 0;    // TODO
        return 0;
    }

    return -ENOENT;
}

static int
capfs_mkdir(const char *path, mode_t mode) {
    return 0;
}

static int
capfs_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int
capfs_opendir(const char *path, struct fuse_file_info *fi) {
    printf("woah!");
    return 0;
}

static int
capfs_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi) {
    return 0;
}

static int
capfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi) {
    return 0;
}

static int
capfs_release(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int
capfs_releasedir(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int
capfs_rmdir(const char *path) {
    return 0;
}

static int
capfs_truncate(const char *path, off_t file_size) {
    return 0;
}

static int
capfs_write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi) {
    return 0;
}

static struct fuse_operations capfs_operations = {
    .access = capfs_access,
    .create = capfs_create,
    .getattr = capfs_getattr,
    .mkdir = capfs_mkdir,
    .open = capfs_open,
    .opendir = capfs_opendir,
    .read = capfs_read,
    .readdir = capfs_readdir,
    .release = capfs_release,
    .releasedir = capfs_releasedir,
    .rmdir = capfs_rmdir,
    .truncate = capfs_truncate,
    .write = capfs_write,
};

void
init(void) {
    fh_init();

    EP_STAT estat = gdp_init(NULL);
    if (!EP_STAT_ISOK(estat)) {
        exit(EX_UNAVAILABLE);
    }
}

int
run(int argc, char *argv[]) {
    init();
    return fuse_main(argc, argv, &capfs_operations, NULL);
}
