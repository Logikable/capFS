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

#include "capfs_file.h"
#include "capfs_util.h"

#include <string.h>

static size_t
capfs_file_inode_ptr(off_t offset) {
    if (offset < DIRECT_PTRS_SIZE) {
        return offset / BLOCK_SIZE;
    }
    return (offset - DIRECT_PTRS_SIZE) / INDIRECT_PTR_SIZE + DIRECT_PTRS;
}

static bool
capfs_file_offset_requires_indirect(off_t offset) {
    return offset >= DIRECT_PTRS_SIZE;
}

// Only well defined for offsets that use indirect pointers (i.e. bigger than
//   DIRECT_PTRS_SIZE)
static size_t
capfs_file_indirect_ptr(off_t offset) {
    if (offset < DIRECT_PTRS_SIZE) {
        return -1;
    }
    return ((offset - DIRECT_PTRS_SIZE) % INDIRECT_PTR_SIZE) / BLOCK_SIZE;
}

EP_STAT
capfs_file_read(capfs_file_t *file, char *buf, size_t size, off_t offset) {
    // GOB -> GIN
    gdp_open_info_t *goi = gdp_open_info_new();
    gdp_gin_t *ginp;
    if (!EP_STAT_ISOK(gdp_gin_open(file->gob, GDP_MODE_RO, goi, &ginp))) {
        goto fail0;
    }

    // Open GIN
    gdp_datum_t *last_record = gdp_datum_new();
    if (!EP_STAT_ISOK(gdp_gin_read_by_recno(ginp, -1, last_record))) {
        goto fail0;
    }

    // Read datum into buffer, get inode
    gdp_buf_t *dbuf = gdp_datum_getbuf(last_record);
    size_t buf_len = gdp_buf_getlength(dbuf);
    if (buf_len < INODE_SIZE + BLOCK_SIZE) {   // returned data is too small
        goto fail1;
    }
    inode_t inode;
    gdp_buf_read(dbuf, (void *) &inode, INODE_SIZE);

    // Get direct ptrs, perform lookups, write to buf
    while (!capfs_file_offset_requires_indirect(offset) && size > 0) {
        size_t ptr = capfs_file_inode_ptr(offset);
        uint32_t recno = inode.direct_ptrs[ptr];
        // Open GIN
        gdp_datum_t *direct_datum = gdp_datum_new();
        if (!EP_STAT_ISOK(gdp_gin_read_by_recno(ginp, recno, direct_datum))) {
            gdp_datum_free(direct_datum);
            goto fail1;
        }
        // Get inode metadata
        gdp_buf_t *direct_buf = gdp_datum_getbuf(direct_datum);
        size_t buf_len = gdp_buf_getlength(direct_buf);
        if (buf_len < INODE_SIZE) {
            gdp_buf_free(direct_buf);
            gdp_datum_free(direct_datum);
            goto fail1;
        }
        inode_t inode2;
        gdp_buf_read(direct_buf, (void *) &inode2, INODE_SIZE);
        // Sanity check
        if ((buf_len < INODE_SIZE + BLOCK_SIZE
                && !inode.has_indirect_block)
            || (buf_len < INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE
                && inode.has_indirect_block)) {
            gdp_buf_free(direct_buf);
            gdp_datum_free(direct_datum);
            goto fail1;
        }
        // Get raw data
        char data_buf[BLOCK_SIZE];
        if (inode.has_indirect_block) {
            size_t buf_size = INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE;
            char temp_buf[buf_size];
            gdp_buf_read(direct_buf, (void *) temp_buf, buf_size);
            memcpy(data_buf, temp_buf + INODE_SIZE + INDIRECT_SIZE, BLOCK_SIZE);
        } else {
            size_t buf_size = INODE_SIZE + BLOCK_SIZE;
            char temp_buf[buf_size];
            gdp_buf_read(direct_buf, (void *) temp_buf, buf_size);
            memcpy(data_buf, temp_buf + INODE_SIZE, BLOCK_SIZE);
        }
        // Copy over relevant portions
        size_t start_byte = offset % BLOCK_SIZE;
        size_t num = min(size, BLOCK_SIZE - start_byte);
        memcpy(buf, data_buf + start_byte, num);

        // Iterate & free
        gdp_buf_free(direct_buf);
        gdp_datum_free(direct_datum);
        buf += num;
        offset += num;
        size -= num;
    }

    // Get indirect ptrs, perform lookups, write to buf
    while (size > 0) {
        size_t ptr = capfs_file_inode_ptr(offset);
        uint32_t indirect_recno = inode.indirect_ptrs[ptr - DIRECT_PTRS];
        // Open GIN
        gdp_datum_t *indirect_datum = gdp_datum_new();
        if (!EP_STAT_ISOK(gdp_gin_read_by_recno(ginp, indirect_recno,
                                                indirect_datum))) {
            gdp_datum_free(indirect_datum);
            goto fail1;
        }
        // Read datum into buffer, get indirect block
        gdp_buf_t *indirect_buf = gdp_datum_getbuf(indirect_datum);
        size_t buf_len = gdp_buf_getlength(indirect_buf);
        size_t temp_buf_size = INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE;
        if (buf_len < temp_buf_size) {
            gdp_buf_free(indirect_buf);
            gdp_datum_free(indirect_datum);
            goto fail1;
        }
        char temp_buf[temp_buf_size];
        gdp_buf_read(indirect_buf, (void *) temp_buf, temp_buf_size);
        char indirect_ptrs[INDIRECT_SIZE];
        memcpy(indirect_ptrs, temp_buf + INODE_SIZE, INDIRECT_SIZE);
        // Iterate within indirect block
        while (capfs_file_inode_ptr(offset) == ptr && size > 0) {

        }
    }

    // Cleanup
    gdp_datum_free(last_record);
    gdp_open_info_free(goi);
    return EP_STAT_OK;

fail1:
    gdp_buf_free(dbuf);
    gdp_datum_free(last_record);
    gdp_open_info_free(goi);
    return EP_STAT_END_OF_FILE;
fail0:
    gdp_datum_free(last_record);
    gdp_open_info_free(goi);
    return EP_STAT_NOT_FOUND;
}

EP_STAT
capfs_file_write(capfs_file_t *file, const char *buf, size_t size,
                 off_t offset) {
    // TODO
    return EP_STAT_OK;
}

void
capfs_file_free(capfs_file_t *file) {
    free(file);
}
