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

// Does not perform writeback
static EP_STAT
capfs_dir_table_insert_entry(capfs_dir_table_t *table, const char *name,
                             bool is_dir, gdp_name_t gob) {
    EP_STAT estat;

    // Make capfs_dir_entry_t
    capfs_dir_entry_t new_entry;
    new_entry.is_dir = is_dir;
    new_entry.valid = true;
    memcpy(new_entry.gob, gob, sizeof(gdp_name_t));
    strcpy(new_entry.name, name);

    // Insert entry into available slot (for parent)
    size_t i = 0;
    for (; i < DIR_ENTRIES; i++) {
        capfs_dir_entry_t entry = table->entries[i];
        if (!entry.valid) {
            memcpy(table->entries + i, &new_entry, DIR_ENTRY_SIZE);
            break;
        }
    }
    // Sanity check
    if (i == DIR_ENTRIES) {
        estat = EP_STAT_OUT_OF_MEMORY;
        goto fail0;
    }
    table->length++;

    return EP_STAT_OK;

fail0:
    return estat;
}

// Should only be called manually! See test/make_root.c
EP_STAT
capfs_dir_make_root(void) {
    EP_STAT estat;

    // Check that it doesn't exist
    capfs_dir_t *dir;
    estat = capfs_dir_open_root(&dir);
    if (EP_STAT_ISOK(estat)) {
        estat = EP_STAT_ASSERT_ABORT;
        goto fail0;
    }

    // Create new file
    capfs_file_t *file;
    estat = capfs_file_create("/", &file);
    EP_STAT_CHECK(estat, goto fail0);

    // Write empty directory
    capfs_dir_table_t table;
    memset(&table, 0, DIR_TABLE_SIZE);
    // Add . and ..
    estat = capfs_dir_table_insert_entry(&table, ".", true, file->gob);
    EP_STAT_CHECK(estat, goto fail1);
    estat = capfs_dir_table_insert_entry(&table, "..", true, file->gob);
    EP_STAT_CHECK(estat, goto fail1);
    // Commit
    estat = capfs_file_write(file, (const char *) &table, DIR_TABLE_SIZE,
                             0);
    EP_STAT_CHECK(estat, goto fail1);

    // Close & Cleanup
    estat = capfs_file_close(file);
    EP_STAT_CHECK(estat, goto fail1);
    capfs_file_free(file);
    return EP_STAT_OK;

fail1:
    capfs_file_free(file);
fail0:
    return estat;
}

EP_STAT
capfs_dir_open_root(capfs_dir_t **dir) {
    EP_STAT estat;    
    capfs_file_t *file;

    estat = capfs_file_open("/", &file);
    EP_STAT_CHECK(estat, goto fail0);
    *dir = capfs_dir_new(file);
    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
capfs_dir_make_step_1(capfs_dir_t *parent, const char *name,
                      capfs_file_t **file, capfs_dir_table_t *parent_table) {
    if (parent == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    // Read and parse parent table
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    gdp_name_t gobs[DIR_ENTRIES];

    estat = capfs_dir_readdir(parent, parent_table, names, gobs);
    EP_STAT_CHECK(estat, goto fail0);

    // Capacity check (in parent)
    if (parent_table->length == DIR_ENTRIES) {
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

    // Create new file (for child)
    estat = capfs_file_create_gob(file);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
capfs_dir_make_step_2(capfs_dir_t *parent, const char *name, bool is_dir,
                      capfs_file_t *file, capfs_dir_table_t *parent_table) {
    EP_STAT estat;

    estat = capfs_dir_table_insert_entry(parent_table, name, is_dir, file->gob);
    EP_STAT_CHECK(estat, goto fail0);

    // Writeback (for parent)
    estat = capfs_file_write(parent->file, (const char *) parent_table,
                             DIR_TABLE_SIZE, 0);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_make_file(capfs_dir_t *parent, const char *name, 
                    capfs_file_t **file) {
    EP_STAT estat;

    capfs_dir_table_t parent_table;
    estat = capfs_dir_make_step_1(parent, name, file, &parent_table);
    EP_STAT_CHECK(estat, goto fail0);

    estat = capfs_dir_make_step_2(parent, name, false, *file, &parent_table);
    EP_STAT_CHECK(estat, goto fail0);

    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_mkdir(capfs_dir_t *parent, const char *name, capfs_dir_t **dir) {
    EP_STAT estat;

    capfs_file_t *file;
    capfs_dir_table_t parent_table;

    estat = capfs_dir_make_step_1(parent, name, &file, &parent_table);
    EP_STAT_CHECK(estat, goto fail0);
    *dir = capfs_dir_new(file);

    // Write empty directory (for child)
    capfs_dir_table_t child_table;
    memset(&child_table, 0, DIR_TABLE_SIZE);
    // Add . and ..
    estat = capfs_dir_table_insert_entry(&child_table, ".", true, file->gob);
    EP_STAT_CHECK(estat, goto fail1);
    estat = capfs_dir_table_insert_entry(&child_table, "..", true,
                                         parent->file->gob);
    EP_STAT_CHECK(estat, goto fail1);
    // Commit
    estat = capfs_file_write(file, (const char *) &child_table, DIR_TABLE_SIZE,
                             0);
    EP_STAT_CHECK(estat, goto fail1);

    estat = capfs_dir_make_step_2(parent, name, true, file, &parent_table);
    EP_STAT_CHECK(estat, goto fail1);

    return EP_STAT_OK;

fail1:
    capfs_dir_free(*dir);
fail0:
    return estat;
}

static EP_STAT
capfs_dir_open_step_1(capfs_dir_t *parent, const char *name, bool *is_dir,
                      gdp_name_t *gob) {
    if (parent == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    // Read in names/gobs
    capfs_dir_table_t table;
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    gdp_name_t gobs[DIR_ENTRIES];
    estat = capfs_dir_readdir(parent, &table, names, gobs);
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

    // Copy data and finish
    *is_dir = table.entries[index].is_dir;
    memcpy(gob, gobs[index], sizeof(gdp_name_t));
    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
capfs_dir_open_step_2(gdp_name_t *gob, capfs_file_t **file) {
    EP_STAT estat;

    // Open the child
    estat = capfs_file_open_gob(*gob, file);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_open_file(capfs_dir_t *parent, const char *name,
                    capfs_file_t **file) {
    EP_STAT estat;

    bool is_dir;
    gdp_name_t gob;
    estat = capfs_dir_open_step_1(parent, name, &is_dir, &gob);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity check
    if (is_dir) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    estat = capfs_dir_open_step_2(&gob, file);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_opendir(capfs_dir_t *parent, const char *name, capfs_dir_t **dir) {
    EP_STAT estat;

    bool is_dir;
    gdp_name_t gob;
    estat = capfs_dir_open_step_1(parent, name, &is_dir, &gob);
    EP_STAT_CHECK(estat, goto fail0);

    // Sanity check
    if (!is_dir) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    capfs_file_t *file;
    estat = capfs_dir_open_step_2(&gob, &file);
    EP_STAT_CHECK(estat, goto fail0);

    *dir = capfs_dir_new(file);
    return EP_STAT_OK;

fail0:
    return estat;
}

// If last token is a directory, does not open it
EP_STAT
capfs_dir_opendir_path(char **path_tokens, size_t num_tokens,
                       capfs_dir_t **dir) {
    EP_STAT estat;

    estat = capfs_dir_open_root(dir);
    EP_STAT_CHECK(estat, goto fail0);

    // Directory exists
    for (size_t i = 0; i < num_tokens - 1; i++) {
        capfs_dir_t *new_dir;
        estat = capfs_dir_opendir(*dir, path_tokens[i], &new_dir);
        EP_STAT_CHECK(estat, goto fail1);
        estat = capfs_dir_closedir(*dir);
        EP_STAT_CHECK(estat, goto fail0);
        *dir = new_dir;
    }
    return EP_STAT_OK;

fail1:
    capfs_dir_closedir(*dir);
fail0:
    return estat;
}

bool
capfs_dir_has_child(capfs_dir_t *parent, const char *name, bool is_dir) {
    EP_STAT estat;

    capfs_dir_table_t table;
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    estat = capfs_dir_readdir(parent, &table, names, NULL);
    EP_STAT_CHECK(estat, goto fail0);

    for (size_t i = 0; i < table.length; i++) {
        if (strcmp(names[i], name) == 0) {
            return is_dir == table.entries[i].is_dir;
        }
    }

fail0:
    return false;
}


// Provide NULL to table or gobs if not needed
EP_STAT
capfs_dir_readdir(capfs_dir_t *dir, capfs_dir_table_t *table,
                  char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1],
                  gdp_name_t gobs[DIR_ENTRIES]) {
    if (dir == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    // Clear names array
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        names[i][0] = '\0';
    }

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
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        capfs_dir_entry_t entry = table->entries[i];
        if (!entry.valid) {
            break;
        }
        strcpy(names[i], entry.name);
        if (gobs != NULL) {
            memcpy(gobs[i], entry.gob, sizeof(gdp_name_t));
        }
    }
    return EP_STAT_OK;

fail0:
    return estat;
}

// Performs writeback
static EP_STAT
capfs_dir_remove_entry(capfs_dir_t *parent, capfs_dir_table_t *table,
                       size_t index) {
    EP_STAT estat;

    // Remove by shifting everything else up
    for (index++; index < table->length; index++) {
        table->entries[index - 1] = table->entries[index];
    }
    // Zero out last one
    memset(table->entries + index - 1, 0, DIR_ENTRY_SIZE);
    table->length--;

    // Writeback
    estat = capfs_file_write(parent->file, (const char *) table,
                             DIR_TABLE_SIZE, 0);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_rename(capfs_dir_t *from, capfs_dir_t *to, const char *from_name,
                 const char *to_name) {
    if (from == NULL || to == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    gdp_name_t gobs[DIR_ENTRIES];

    // Get "from" table + gob (checks from_name exists)
    capfs_dir_table_t from_table;
    estat = capfs_dir_readdir(from, &from_table, names, gobs);
    EP_STAT_CHECK(estat, goto fail0);

    size_t from_index = 0;
    for (; from_index < from_table.length; from_index++) {
        if (strcmp(names[from_index], from_name) == 0) {
            break;
        }
    }
    if (from_index == from_table.length) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    // Get "to" table (checks to_name doesn't exist)
    capfs_dir_table_t to_table;
    estat = capfs_dir_readdir(to, &to_table, names, NULL);
    EP_STAT_CHECK(estat, goto fail0);

    size_t to_index = 0;
    for (; to_index < to_table.length; to_index++) {
        if (strcmp(names[to_index], to_name) == 0) {
            break;
        }
    }
    if (to_index != to_table.length) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    // Sanity check
    if (to_table.length == DIR_ENTRIES) {
        estat = EP_STAT_OUT_OF_MEMORY;
        goto fail0;
    }

    // Insert into to_table, write back
    estat = capfs_dir_table_insert_entry(&to_table, to_name, false, 
                                         gobs[from_index]);
    EP_STAT_CHECK(estat, goto fail0);
    estat = capfs_file_write(to->file, (const char *) &to_table, DIR_TABLE_SIZE,
                             0);
    EP_STAT_CHECK(estat, goto fail0);

    // Remove from from_table, write back
    estat = capfs_dir_remove_entry(from, &from_table, from_index);
    EP_STAT_CHECK(estat, goto fail0);

    return EP_STAT_OK;

fail0:
    return estat;
}

static EP_STAT
capfs_dir_remove_step_1(capfs_dir_t *parent, const char *name,
                        capfs_dir_table_t *table, size_t *index) {
    if (parent == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    // Read parent contents
    char names[DIR_ENTRIES][FILE_NAME_MAX_LEN + 1];
    estat = capfs_dir_readdir(parent, table, names, NULL);
    EP_STAT_CHECK(estat, goto fail0);

    // Find name
    for (; *index < DIR_ENTRIES; (*index)++) {
        if (strcmp(names[*index], name) == 0) {
            break;
        }
    }
    if (*index == DIR_ENTRIES) {
        estat = EP_STAT_NOT_FOUND;
        goto fail0;
    }
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_remove_file(capfs_dir_t *parent, const char *name) {
    EP_STAT estat;

    capfs_dir_table_t table;
    size_t index = 0;
    estat = capfs_dir_remove_step_1(parent, name, &table, &index);
    EP_STAT_CHECK(estat, goto fail0);

    // Check that it's not a directory
    if (table.entries[index].is_dir) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    estat = capfs_dir_remove_entry(parent, &table, index);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

EP_STAT
capfs_dir_rmdir(capfs_dir_t *parent, const char *name) {
    EP_STAT estat;

    capfs_dir_table_t table;
    size_t index = 0;
    estat = capfs_dir_remove_step_1(parent, name, &table, &index);
    EP_STAT_CHECK(estat, goto fail0);

    // Check that it's a directory
    if (!table.entries[index].is_dir) {
        estat = EP_STAT_INVALID_ARG;
        goto fail0;
    }

    estat = capfs_dir_remove_entry(parent, &table, index);
    EP_STAT_CHECK(estat, goto fail0);
    return EP_STAT_OK;

fail0:
    return estat;
}

// Does not free for you
EP_STAT
capfs_dir_closedir(capfs_dir_t *dir) {
    if (dir == NULL) {
        return EP_STAT_INVALID_ARG;
    }
    EP_STAT estat;

    estat = capfs_file_close(dir->file);
    EP_STAT_CHECK(estat, goto fail0);
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
    if (dir == NULL) {
        return;
    }
    capfs_file_free(dir->file);
    free(dir);
}
