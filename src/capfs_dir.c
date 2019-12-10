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

#include "capfs_dir.h"

#include <string.h>

EP_STAT
capfs_dir_get_root(capfs_dir_t **dir) {
    EP_STAT estat;    
    capfs_file_t *file;

    estat = capfs_file_open("/", &file);
    EP_STAT_CHECK(estat, goto fail0);
    *dir = capfs_dir_new(file);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_mkdir(capfs_dir_t *parent, const char *path, const char *name,
                capfs_dir_t **dir) {
    EP_STAT estat;

    // Read and parse parent table
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        names[i][0] = '\0';
    }
    gdp_name_t gobs[DIR_ENTRIES];
    capfs_dir_table_t parent_table;

    estat = capfs_dir_readdir(parent, &parent_table, names, gobs);
    EP_STAT_CHECK(estat, goto fail0);

    // Capacity check (in parent)
    if (parent_table.length == DIR_ENTRIES) {
        estat = EP_STAT_OUT_OF_MEMORY;
        goto fail0;
    }

    // Check existence (in parent)
    size_t index = 0;
    for (; index < DIR_ENTRIES; index++) {
        if (strcmp(names[index], name) == 0) {
            break;
        }
    }
    if (index != DIR_ENTRIES) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    // Make capfs_dir_table_t (for child)
    capfs_dir_table_t child_table;
    memset(&child_table, 0, DIR_TABLE_SIZE);

    // Create new file (for child)
    capfs_file_t *file;
    estat = capfs_file_create(path, &file);
    EP_STAT_CHECK(estat, goto fail0);
    *dir = capfs_dir_new(file);

    // Make capfs_dir_entry_t (for parent)
    capfs_dir_entry_t new_entry;
    new_entry.is_dir = true;
    new_entry.valid = true;
    memcpy(&new_entry.gob, &(file->gob), sizeof(gdp_name_t));
    strcpy(new_entry.name, name);

    // Insert entry into available slot (for parent)
    size_t i = 0;
    for (; i < DIR_ENTRIES; i++) {
        capfs_dir_entry_t entry = parent_table.entries[i];
        if (!entry.valid) {
            memcpy(parent_table.entries + i, &new_entry, DIR_ENTRY_SIZE);
            break;
        }
    }
    // Sanity check
    if (i == DIR_ENTRIES) {
        estat = EP_STAT_OUT_OF_MEMORY;
        goto fail1;
    }
    parent_table.length++;

    // Writeback (for parent)
    estat = capfs_file_write(parent->file, (const char *) &parent_table,
                             DIR_TABLE_SIZE, 0);
    EP_STAT_CHECK(estat, goto fail1);

    return EP_STAT_OK;

fail1:
    capfs_dir_free(*dir);
fail0:
    return estat;
}

EP_STAT
capfs_dir_opendir(capfs_dir_t *parent, const char *name, capfs_dir_t **dir) {
    EP_STAT estat;

    // Read in names/gobs
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        names[i][0] = '\0';
    }
    gdp_name_t gobs[DIR_ENTRIES];
    estat = capfs_dir_readdir(parent, NULL, names, gobs);
    EP_STAT_CHECK(estat, goto fail0);

    // Verify child is there
    size_t index = 0;
    for (; index < DIR_ENTRIES; index++) {
        if (strcmp(names[index], name) == 0) {
            break;
        }
    }
    if (index == DIR_ENTRIES) {
        estat = EP_STAT_NOT_FOUND;
        goto fail0;
    }

    // Open the child
    capfs_file_t *file;
    estat = capfs_file_open_gob(gobs[index], &file);
    EP_STAT_CHECK(estat, goto fail0);

    *dir = capfs_dir_new(file);
    return EP_STAT_OK;

fail0:
    return estat;
}


EP_STAT
capfs_dir_readdir(capfs_dir_t *dir, capfs_dir_table_t *table,
                  char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1],
                  gdp_name_t gobs[DIR_ENTRIES]) {
    EP_STAT estat;

    // Read file contents
    char block_buf[DIR_TABLE_SIZE];
    estat = capfs_file_read(dir->file, block_buf, DIR_TABLE_SIZE, 0);
    EP_STAT_CHECK(estat, goto fail0);

    // Copy into capfs_dir_table_t
    if (table == NULL) {
        capfs_dir_table_t _table;
        table = &_table;
    }
    memcpy(table, block_buf, DIR_TABLE_SIZE);

    // Copy table entry names and gobs
    size_t index = 0;
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        capfs_dir_entry_t entry = table->entries[i];
        if (!entry.valid) {
            continue;
        }
        strcpy((char *) names + index, entry.name);
        if (gobs != NULL) {
            memcpy(gobs + index, entry.gob, sizeof(gdp_name_t));
        }
        index++;
    }
    return EP_STAT_OK;

fail0:
    return estat;
}

capfs_dir_t *
capfs_dir_new(capfs_file_t *file) {
    capfs_dir_t *dir = calloc(sizeof(capfs_dir_t), 1);
    dir->file = file;
    return dir;
}

void
capfs_dir_free(capfs_dir_t *dir) {
    capfs_file_free(dir->file);
    free(dir);
}
