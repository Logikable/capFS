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

typedef struct capfs_dir {
    capfs_file_t *file;
} capfs_dir_t;

typedef struct capfs_dir_entry {
    unsigned int is_dir : 1;
    unsigned int length : 8;
    unsigned int padding : 23;
    char gob[32];
    char name[128];
} capfs_dir_entry_t;

capfs_dir_t *capfs_dir_get_root(void);
bool capfs_dir_has_child(capfs_dir_t *dir, const char *path);
EP_STAT capfs_dir_get_child(capfs_dir_t *dir, const char *path,
                            capfs_dir_t **child);
void capfs_dir_free(capfs_dir_t *dir);

#endif // _CAPFS_DIR_H_
