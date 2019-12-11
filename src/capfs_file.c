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

#include <string.h>

#include "capfs_util.h"

// offset -> inode ptrs index
// Returns an index as if indirect ptrs began indexing at DIRECT_PTRS
static size_t
capfs_file_inode_ptr(off_t offset) {
    if (offset < DIRECT_PTRS_SIZE) {
        return offset / BLOCK_SIZE;
    }
    return (offset - DIRECT_PTRS_SIZE) / INDIRECT_PTR_SIZE + DIRECT_PTRS;
}

// Returns whether offset needs an indirect pointer
static bool
capfs_file_offset_requires_indirect(off_t offset) {
    return offset >= DIRECT_PTRS_SIZE;
}

// offset -> direct ptr index within an indirect block
// Only well defined for offsets that use indirect pointers (i.e. bigger than
//   DIRECT_PTRS_SIZE)
static size_t
capfs_file_indirect_ptr(off_t offset) {
    if (offset < DIRECT_PTRS_SIZE) {
        return -1;
    }
    return ((offset - DIRECT_PTRS_SIZE) % INDIRECT_PTR_SIZE) / BLOCK_SIZE;
}

static void
capfs_file_inode_print(inode_t *inode) {
    printf("Inode:\n");
    printf("is_dir: %d\n", inode->is_dir);
    printf("has_indirect_block: %d\n", inode->has_indirect_block);
    printf("recno: %d\n", inode->recno);
    printf("length: %ld\n", inode->length);
    printf("direct_ptrs: ");
    for (size_t i = 0; i < DIRECT_PTRS; i++) {
        if (inode->direct_ptrs[i] != 0) {
            printf("%d, ", inode->direct_ptrs[i]);
        }
    }
    printf("\n");
    printf("indirect_ptrs: ");
    for (size_t i = 0; i < INDIRECT_PTRS; i++) {
        if (inode->indirect_ptrs[i] != 0) {
            printf("%d, ", inode->indirect_ptrs[i]);
        }
    }
    printf("\n");
}

static EP_STAT
capfs_file_read_block_from_recno(size_t recno, gdp_gin_t *ginp,
                                 char **buf, size_t *size, off_t *offset) {
    EP_STAT estat;

    // Open GIN
    gdp_datum_t *direct_datum = gdp_datum_new();
    estat = gdp_gin_read_by_recno(ginp, recno, direct_datum);
    EP_STAT_CHECK(estat, goto fail0);

    // Get inode metadata
    gdp_buf_t *direct_buf = gdp_datum_getbuf(direct_datum);
    size_t buf_len = gdp_buf_getlength(direct_buf);
    if (buf_len < INODE_SIZE) {
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }
    inode_t inode2;
    gdp_buf_read(direct_buf, (void *) &inode2, INODE_SIZE);

    // Sanity check
    if ((buf_len < INODE_SIZE + BLOCK_SIZE
            && !inode2.has_indirect_block)
        || (buf_len < INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE
            && inode2.has_indirect_block)) {
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }

    // Get raw data
    char data_buf[BLOCK_SIZE];
    if (inode2.has_indirect_block) {
        size_t buf_size = INDIRECT_SIZE + BLOCK_SIZE;
        char temp_buf[buf_size];
        gdp_buf_read(direct_buf, (void *) temp_buf, buf_size);
        memcpy(data_buf, temp_buf + INDIRECT_SIZE, BLOCK_SIZE);
    } else {
        size_t buf_size = BLOCK_SIZE;
        char temp_buf[buf_size];
        gdp_buf_read(direct_buf, (void *) temp_buf, buf_size);
        memcpy(data_buf, temp_buf, BLOCK_SIZE);
    }

    // Copy over relevant portions
    size_t start_byte = *offset % BLOCK_SIZE;
    size_t num = min(*size, BLOCK_SIZE - start_byte);
    memcpy(*buf, data_buf + start_byte, num);

    // Iterate & cleanup
    gdp_datum_free(direct_datum);
    *buf += num;
    *offset += num;
    *size -= num;
    return EP_STAT_OK;

fail0:
    gdp_datum_free(direct_datum);
    return estat;
}

static EP_STAT
capfs_file_read_indirect_from_recno(size_t indirect_recno, gdp_gin_t *ginp,
        uint32_t indirect_block[DIRECT_IN_INDIRECT]) {
    EP_STAT estat;

    // Open GIN
    gdp_datum_t *indirect_datum = gdp_datum_new();
    estat = gdp_gin_read_by_recno(ginp, indirect_recno, indirect_datum);
    EP_STAT_CHECK(estat, goto fail0);

    // Read datum into buffer, get indirect block
    gdp_buf_t *indirect_buf = gdp_datum_getbuf(indirect_datum);
    size_t buf_len = gdp_buf_getlength(indirect_buf);
    if (buf_len < INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE) {
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }
    char temp_buf[INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE];
    gdp_buf_read(indirect_buf, (void *) temp_buf,
                 INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE);
    memcpy(indirect_block, temp_buf + INODE_SIZE, INDIRECT_SIZE);

    // Cleanup
    gdp_datum_free(indirect_datum);
    return EP_STAT_OK;

fail0:
    gdp_datum_free(indirect_datum);
    return estat;
}

static EP_STAT
capfs_file_read_inode(gdp_gin_t *ginp, inode_t *inode, gdp_hash_t **prevhash) {
    EP_STAT estat;

    // Read last record
    gdp_datum_t *last_record = gdp_datum_new();
    estat = gdp_gin_read_by_recno(ginp, -1, last_record);
    EP_STAT_CHECK(estat, goto fail0);

    // Read datum into buffer, get inode
    gdp_buf_t *dbuf = gdp_datum_getbuf(last_record);
    size_t buf_len = gdp_buf_getlength(dbuf);
    if (buf_len < INODE_SIZE + BLOCK_SIZE) {   // returned data is too small
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }
    gdp_buf_read(dbuf, (void *) inode, INODE_SIZE);
    if (prevhash != NULL) {
        *prevhash = gdp_datum_hash(last_record, ginp);
    }
    gdp_datum_free(last_record);
    return EP_STAT_OK;

fail0:
    gdp_datum_free(last_record);
    return estat;
}

EP_STAT
capfs_file_read(capfs_file_t *file, char *buf, size_t size, off_t offset) {
    if (file == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;
    gdp_gin_t *ginp = file->ginp;

    // Read inode
    inode_t inode;
    estat = capfs_file_read_inode(ginp, &inode, NULL);
    EP_STAT_CHECK(estat, goto fail0);

    // Error checking
    if (inode.length < offset + size) {
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }

    // Get direct ptrs, perform lookups, write to buf
    while (!capfs_file_offset_requires_indirect(offset) && size > 0) {
        size_t ptr = capfs_file_inode_ptr(offset);
        uint32_t recno = inode.direct_ptrs[ptr];
        estat = capfs_file_read_block_from_recno(recno, ginp, &buf, &size,
                                                 &offset);
        EP_STAT_CHECK(estat, goto fail0);
    }

    // Get indirect block, perform lookups, write to buf
    while (size > 0) {
        size_t ptr = capfs_file_inode_ptr(offset);
        uint32_t indirect_recno = inode.indirect_ptrs[ptr - DIRECT_PTRS];
        // Get indirect block
        uint32_t indirect_block[DIRECT_IN_INDIRECT];
        estat = capfs_file_read_indirect_from_recno(indirect_recno, ginp,
                                                    indirect_block);
        EP_STAT_CHECK(estat, goto fail0);
        // Iterate within indirect block
        while (capfs_file_inode_ptr(offset) == ptr && size > 0) {
            size_t index = capfs_file_indirect_ptr(offset);
            uint32_t recno = indirect_block[index];
            estat = capfs_file_read_block_from_recno(recno, ginp, &buf, &size,
                                                     &offset);
            EP_STAT_CHECK(estat, goto fail0);
        }
    }

    // Cleanup
    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
capfs_file_write_record(gdp_gin_t *ginp, gdp_hash_t *prevhash,
                        gdp_hash_t **newhash, inode_t *inode,
                        uint32_t indirect_block[DIRECT_IN_INDIRECT],
                        char data_block[BLOCK_SIZE]) {
    EP_STAT estat;

    gdp_datum_t *datum = gdp_datum_new();
    gdp_buf_t *buf = gdp_datum_getbuf(datum);

    if (indirect_block == NULL) {
        size_t size = INODE_SIZE + BLOCK_SIZE;
        char payload[size];
        memcpy(payload, inode, INODE_SIZE);
        memcpy(payload + INODE_SIZE, data_block, BLOCK_SIZE);
        gdp_buf_write(buf, (void *) payload, size);
    } else {
        size_t size = INODE_SIZE + INDIRECT_SIZE + BLOCK_SIZE;
        char payload[size];
        memcpy(payload, inode, INODE_SIZE);
        memcpy(payload + INODE_SIZE, indirect_block, INDIRECT_SIZE);
        memcpy(payload + INODE_SIZE + INDIRECT_SIZE, data_block, BLOCK_SIZE);
        gdp_buf_write(buf, (void *) payload, size);
    }

    estat = gdp_gin_append(ginp, datum, prevhash);
    EP_STAT_CHECK(estat, goto fail0);
    *newhash = gdp_datum_hash(datum, ginp);

    gdp_datum_free(datum);
    return EP_STAT_OK;

fail0:
    gdp_datum_free(datum);
    return estat;
}

static EP_STAT
capfs_file_write_block(gdp_gin_t *ginp, gdp_hash_t **prevhash, uint32_t recno,
                       inode_t *inode,
                       uint32_t indirect_block[DIRECT_IN_INDIRECT],
                       const char **buf, size_t *size, off_t *offset) {
    EP_STAT estat;

    size_t local_offset = *offset % BLOCK_SIZE;
    size_t num;
    char write_buf[BLOCK_SIZE];
    // Handling the first block (if not aligned) -- read + copy before write
    if (local_offset != 0) {
        // Dummy variables so the originals are not modified
        size_t sz = *size;
        off_t off = *offset;
        estat = capfs_file_read_block_from_recno(recno, ginp,
                                                 (char **) &write_buf,
                                                 &sz, &off);
        EP_STAT_CHECK(estat, goto fail0);
        // Number of bytes we're writing
        num = min(*size, BLOCK_SIZE - local_offset);
        memcpy(write_buf + local_offset, *buf, num);
    } else {    // just write
        num = min(*size, BLOCK_SIZE);
        memcpy(write_buf, *buf, num);
    }

    // Update inode
    recno = inode->recno + 1;
    inode->recno = recno;
    inode->has_indirect_block = false;
    if (*offset + num > inode->length) {
        // Check if write exceeds file size
        inode->length = *offset + num;
    }

    if (indirect_block == NULL) {
        size_t ptr = capfs_file_inode_ptr(*offset);
        inode->direct_ptrs[ptr] = recno;
    } else {
        size_t inode_ptr = capfs_file_inode_ptr(*offset);
        size_t indirect_ptr = capfs_file_indirect_ptr(*offset);
        indirect_block[indirect_ptr] = recno;
        inode->indirect_ptrs[inode_ptr - DIRECT_PTRS] = recno;
    }
    // Write to log
    estat = capfs_file_write_record(ginp, *prevhash, prevhash, inode,
                                    indirect_block, write_buf);
    EP_STAT_CHECK(estat, goto fail0);

    *offset += num;
    *buf += num;
    *size -= num;
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_file_write(capfs_file_t *file, const char *buf, size_t size,
                 off_t offset) {
    if (file == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;
    gdp_gin_t *ginp = file->ginp;

    // Read inode
    inode_t inode;
    gdp_hash_t *prevhash;
    estat = capfs_file_read_inode(ginp, &inode, &prevhash);
    EP_STAT_CHECK(estat, goto fail0);

    // Error checking
    if (offset > inode.length) {
        estat = EP_STAT_END_OF_FILE;
        goto fail0;
    }

    // Handle direct ptrs
    while (!capfs_file_offset_requires_indirect(offset) && size > 0) {
        size_t ptr = capfs_file_inode_ptr(offset);
        uint32_t recno = inode.direct_ptrs[ptr];
        estat = capfs_file_write_block(ginp, &prevhash, recno, &inode,
                                       NULL, &buf, &size, &offset);
        EP_STAT_CHECK(estat, goto fail0);
    }

    // Handle indirect ptrs
    while (size > 0) {
        // Handling the first indirect block (if not aligned) -- read + copy 1st
        size_t indirect_offset =
            (offset - DIRECT_PTRS_SIZE) % INDIRECT_PTR_SIZE;
        size_t indirect_ptr = capfs_file_inode_ptr(offset);
        uint32_t indirect_block[DIRECT_IN_INDIRECT];
        if (indirect_offset != 0) {
            uint32_t indirect_recno = inode.indirect_ptrs[ \
                indirect_ptr - DIRECT_PTRS];
            // Read indirect block
            estat = capfs_file_read_indirect_from_recno(indirect_recno, ginp,
                                                        indirect_block);
            EP_STAT_CHECK(estat, goto fail0);
        } else {    // just write data
            memset(indirect_block, 0, INDIRECT_SIZE);
        }
        // Iterate within indirect block
        while (capfs_file_inode_ptr(offset) == indirect_ptr && size > 0) {
            size_t index = capfs_file_indirect_ptr(offset);
            uint32_t recno = indirect_block[index];
            estat = capfs_file_write_block(ginp, &prevhash, recno, &inode,
                                           indirect_block,
                                           &buf, &size, &offset);
            EP_STAT_CHECK(estat, goto fail0);
        }
    }
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_file_get_length(capfs_file_t *file, size_t *length) {
    if (file == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;
    gdp_gin_t *ginp = file->ginp;

    // Read inode
    inode_t inode;
    gdp_hash_t *prevhash;
    estat = capfs_file_read_inode(ginp, &inode, &prevhash);
    EP_STAT_CHECK(estat, goto fail0);

    *length = inode.length;
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_file_truncate(capfs_file_t *file, off_t file_size) {
    if (file == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;
    gdp_gin_t *ginp = file->ginp;

    // Read inode
    inode_t inode;
    gdp_hash_t *prevhash;
    estat = capfs_file_read_inode(ginp, &inode, &prevhash);
    EP_STAT_CHECK(estat, goto fail0);

    // Error checking
    if (file_size > inode.length) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    // Update length + metadata
    inode.length = file_size;
    inode.recno++;
    inode.has_indirect_block = false;

    // Write in an empty block
    char data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);
    estat = capfs_file_write_record(ginp, prevhash, &prevhash, &inode, NULL,
                                    data_block);
    EP_STAT_CHECK(estat, goto fail0);

    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
_capfs_file_create(const char *name, capfs_file_t **file) {
    EP_STAT estat;

    // Create GCI - prep for creating DataCapsule
    gdp_create_info_t *gci = gdp_create_info_new();
    estat = gdp_create_info_set_creator(gci, "CapFS",
                                        "fa19.cs262.eecs.berkeley.edu");
    EP_STAT_CHECK(estat, goto fail0);
    // Create a new key, save it in KEYS
    estat = gdp_create_info_new_owner_key(gci, NULL, NULL, 0, NULL, "null");
    EP_STAT_CHECK(estat, goto fail0);

    // Create DataCapsule
    gdp_gin_t *ginp;
    estat = gdp_gin_create(gci, name, &ginp);
    EP_STAT_CHECK(estat, goto fail0);

    gdp_name_t gob;
    memcpy(&gob, gdp_gin_getname(ginp), sizeof(gdp_name_t));

    // Close and re-open the capsule (workaround)
    estat = gdp_gin_close(ginp);
    EP_STAT_CHECK(estat, goto fail1);
    gdp_open_info_t *goi = gdp_open_info_new();
    estat = gdp_gin_open(gob, GDP_MODE_RA, goi, &ginp);
    EP_STAT_CHECK(estat, goto fail2);

    *file = capfs_file_new(gob);
    (*file)->ginp = ginp;

    // Get the hash of the zeroth log record
    gdp_datum_t *datum = gdp_datum_new();
    estat = gdp_gin_read_by_recno(ginp, 0, datum);
    EP_STAT_CHECK(estat, goto fail3);
    gdp_hash_t *prevhash = gdp_datum_hash(datum, ginp);

    // Write inode
    inode_t inode;
    memset(&inode, 0, sizeof(inode_t));
    inode.is_dir = false;
    inode.has_indirect_block = false;
    inode.recno = 1;
    inode.length = 0;
    // Write data
    char data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);
    // Combine
    char payload[INODE_SIZE + BLOCK_SIZE];
    memcpy(payload, (void *) &inode, INODE_SIZE);
    memcpy(payload + INODE_SIZE, data_block, BLOCK_SIZE);

    // Write first record
    gdp_datum_reset(datum);
    gdp_buf_t *buf = gdp_datum_getbuf(datum);
    gdp_buf_write(buf, payload, INODE_SIZE + BLOCK_SIZE);

    estat = gdp_gin_append(ginp, datum, prevhash);
    EP_STAT_CHECK(estat, goto fail3);

    // Cleanup
    gdp_datum_free(datum);
    gdp_create_info_free(&gci);
    return EP_STAT_OK;

fail3:
    gdp_datum_free(datum);
    capfs_file_free(*file);
fail2:
    gdp_open_info_free(goi);
fail1:
    gdp_gin_delete(ginp);
fail0:
    gdp_create_info_free(&gci);
    return estat;
}

EP_STAT
capfs_file_create(const char *path, capfs_file_t **file) {
    EP_STAT estat;

    char human_name[256];
    get_human_name(path, human_name);
    estat = _capfs_file_create(human_name, file);
    EP_STAT_CHECK(estat, goto fail0);

    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_file_create_gob(capfs_file_t **file) {
    EP_STAT estat;

    estat = _capfs_file_create(NULL, file);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_file_open(const char *path, capfs_file_t **file) {
    EP_STAT estat;

    char human_name[256];
    get_human_name(path, human_name);

    gdp_name_t gob;
    estat = gdp_parse_name(human_name, gob);
    EP_STAT_CHECK(estat, goto fail0);

    return capfs_file_open_gob(gob, file);

fail0:
    return estat;
}

EP_STAT
capfs_file_open_gob(gdp_name_t gob, capfs_file_t **file) {
    EP_STAT estat;

    // Make goi
    gdp_open_info_t *goi = gdp_open_info_new();
    *file = capfs_file_new(gob);

    // Open
    gdp_gin_t *ginp;
    estat = gdp_gin_open(gob, GDP_MODE_RA, goi, &ginp);
    EP_STAT_CHECK(estat, goto fail0);
    (*file)->ginp = ginp;

    gdp_open_info_free(goi);
    return EP_STAT_OK;

fail0:
    capfs_file_free(*file);
    gdp_open_info_free(goi);
    return estat;
}

// Does not free for you
EP_STAT
capfs_file_close(capfs_file_t *file) {
    if (file == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    estat = gdp_gin_close(file->ginp);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

capfs_file_t *
capfs_file_new(const gdp_name_t gob) {
    capfs_file_t *file = calloc(sizeof(capfs_file_t), 1);
    memcpy(file->gob, gob, sizeof(gdp_name_t));
    return file;
}

void
capfs_file_free(capfs_file_t *file) {
    if (file == NULL) {
        return;
    }
    free(file);
}
