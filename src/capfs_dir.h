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

#ifndef _CAPFS_DIR_H_
#define _CAPFS_DIR_H_

#include <ep/ep.h>
#include <gdp/gdp.h>

#include "capfs_file.h"

// Numerical limit
#define DIR_ENTRIES 186
// Bytes
#define DIR_ENTRY_SIZE (16 + 32 + FILE_NAME_MAX_LEN + 1)
// Just under 32KB (should be less than BLOCK_SIZE)
#define DIR_ENTRIES_SIZE (DIR_ENTRIES * DIR_ENTRY_SIZE)
// 32 bytes
#define DIR_META_SIZE (BLOCK_SIZE - DIR_ENTRIES_SIZE)
// Should be exactly BLOCK_SIZE
#define DIR_TABLE_SIZE (DIR_ENTRIES_SIZE + DIR_META_SIZE)

typedef struct capfs_dir {
    capfs_file_t *file;
} capfs_dir_t;

typedef struct capfs_dir_entry {
    unsigned is_dir : 1;
    unsigned valid : 1;
    unsigned padding1 : 6;

    unsigned char padding2[15];
    unsigned char gob[32];
    char name[FILE_NAME_MAX_LEN + 1];
} capfs_dir_entry_t;

typedef struct capfs_dir_table {
    uint8_t length;
    unsigned char padding[DIR_META_SIZE - 1];
    capfs_dir_entry_t entries[DIR_ENTRIES];
} capfs_dir_table_t;

EP_STAT capfs_dir_make_root(void);
EP_STAT capfs_dir_open_root(capfs_dir_t **dir);
EP_STAT capfs_dir_make_file(capfs_dir_t *parent, const char *path,
                            const char *name, capfs_file_t **file);
EP_STAT capfs_dir_mkdir(capfs_dir_t *parent, const char *path, const char *name,
                        capfs_dir_t **dir);
EP_STAT capfs_dir_open_file(capfs_dir_t *parent, const char *name,
                            capfs_file_t **file);
EP_STAT capfs_dir_opendir(capfs_dir_t *parent, const char *name,
                          capfs_dir_t **dir);
EP_STAT capfs_dir_readdir(capfs_dir_t *dir, capfs_dir_table_t *table,
                          char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1],
                          gdp_name_t gobs[DIR_ENTRIES]);
EP_STAT capfs_dir_remove_file(capfs_dir_t *parent, const char *name);
EP_STAT capfs_dir_rmdir(capfs_dir_t *parent, const char *name);
EP_STAT capfs_dir_closedir(capfs_dir_t *dir);
capfs_dir_t *capfs_dir_new(capfs_file_t *file);
void capfs_dir_free(capfs_dir_t *dir);

#endif // _CAPFS_DIR_H_
