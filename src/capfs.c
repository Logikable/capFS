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

#define FUSE_USE_VERSION 30
#include <fuse.h>

#include "capfs_util.h"

static int
capfs_access(const char *path, int mode) {
    return 0;
}

const char *FILE_PREFIX = "edu.berkeley.eecs.cs262.fa19.";

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

    // Create GCI - prep for creating DataCapsule
    gdp_create_info_t *gci = gdp_create_info_new();
    if (!EP_STAT_ISOK(gdp_create_info_set_creator(gci, "CapFS",
                      "fa19.cs262.eecs.berkeley.edu"))) {
        goto fail0;
    }

    // Get a new file handler
    fh_entry_t *fh;
    if (!EP_STAT_ISOK(fh_new(path, &fh))) {
        goto fail0;
    }
    fi->fh = fh->fh;

    // Create DataCapsule
    char human_name[256];
    strcpy(human_name, FILE_PREFIX);
    strcat(human_name, path);

    gdp_gin_t *ginp;
    if (!EP_STAT_ISOK(gdp_gin_create(gci, human_name, &ginp))) {
        goto fail1;
    }
    // Store GOB
    memcpy(fh->gob, *gdp_gin_getname(ginp), 32);
    // Cleanup
    gdp_create_info_free(&gci);
    return 0;

fail1:
    fh_free(fh->fh);
fail0:
    gdp_create_info_free(&gci);
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

int main(int argc, char *argv[]) {
    fh_init();

    EP_STAT estat = gdp_init(NULL);
    if (!EP_STAT_ISOK(estat)) {
        exit(EX_UNAVAILABLE);
    }
    return fuse_main(argc, argv, &capfs_operations, NULL);
}
