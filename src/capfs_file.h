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

#ifndef _CAPFS_FILE_H_
#define _CAPFS_FILE_H_

// Bump the final number when creating a fresh file system
#define FILE_PREFIX "edu.berkeley.eecs.cs262.fa19.capfs.2."

#define FILE_NAME_MAX_LEN 127

// In bytes
#define INODE_SIZE (8 * 1024)
#define INDIRECT_SIZE (8 * 1024)
#define BLOCK_SIZE (32 * 1024)
#define INODE_METADATA_SIZE 16
// Number of ptrs
#define DIRECT_PTRS ((INODE_SIZE / 4) * 3 / 4)
#define INDIRECT_PTRS ((INODE_SIZE - INODE_METADATA_SIZE - DIRECT_PTRS * 4) / 4)
#define DIRECT_IN_INDIRECT (INDIRECT_SIZE / 4)
// In bytes
// Amount stored in just direct pointers: 48MB
#define DIRECT_PTRS_SIZE (DIRECT_PTRS * BLOCK_SIZE)
// Size of one indirect ptr
#define INDIRECT_PTR_SIZE (DIRECT_IN_INDIRECT * BLOCK_SIZE)
// Roughly 32GB
// #define MAX_FILE_SIZE (DIRECT_PTRS_SIZE + INDIRECT_PTRS * INDIREC_PTR_SIZE)

#include <ep/ep.h>
#include <gdp/gdp.h>

typedef struct capfs_file {
    gdp_name_t gob;
    gdp_gin_t *ginp;
} capfs_file_t;

typedef struct inode {
    unsigned is_dir : 1;            // File data
    unsigned has_indirect_block: 1; // Record data
    unsigned padding1 : 6;

    unsigned int recno;             // Record data
    unsigned long length;           // File data
    unsigned char padding2[3];  // Should be INODE_METADATA_SIZE - size of meta
    uint32_t direct_ptrs[DIRECT_PTRS];
    uint32_t indirect_ptrs[INDIRECT_PTRS]; 
} inode_t;

EP_STAT capfs_file_read(capfs_file_t *file, char *buf, size_t size,
                        off_t offset);
EP_STAT capfs_file_write(capfs_file_t *file, const char *buf, size_t size,
                         off_t offset);
EP_STAT capfs_file_get_length(capfs_file_t *file, size_t *length);
EP_STAT capfs_file_truncate(capfs_file_t *file, off_t file_size);
EP_STAT capfs_file_create(const char *path, capfs_file_t **file);
// Creates a file with no human_name, but is still accessible by gob
EP_STAT capfs_file_create_gob(capfs_file_t **file);
EP_STAT capfs_file_open(const char *path, capfs_file_t **file);
EP_STAT capfs_file_open_gob(gdp_name_t gob, capfs_file_t **file);
EP_STAT capfs_file_close(capfs_file_t *file);
capfs_file_t *capfs_file_new(const gdp_name_t gob);
void capfs_file_free(capfs_file_t *file);

#endif // _CAPFS_FILE_H_
