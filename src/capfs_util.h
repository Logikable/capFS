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

#ifndef _CAPFS_UTIL_H_
#define _CAPFS_UTIL_H_

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#include <ep/ep.h>
#include <gdp/gdp.h>

#include "capfs_dir.h"
#include "capfs_file.h"

typedef struct fh_entry {
    uint64_t fh;
    bool valid;
    bool is_dir;
    uint16_t ref;
    union {
        capfs_dir_t *dir;
        capfs_file_t *file;
    };
} fh_entry_t;

void fh_init(void);
EP_STAT fh_new(fh_entry_t **fh);
EP_STAT fh_get(uint64_t fh, fh_entry_t **fh_ent);
EP_STAT fh_get_by_gob(gdp_name_t gob, fh_entry_t **fh_ent);
void fh_free(uint64_t fh);

size_t split_path(const char *path, char ***tokens);
void free_tokens(char **tokens);

void get_human_name(const char *path, char human_name[256]);

#endif // _CAPFS_UTIL_H_
