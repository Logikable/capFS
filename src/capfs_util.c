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

#include "capfs_util.h"

#include <stdlib.h>
#include <string.h>

#include <ep/ep_app.h>

#include "capfs_file.h"

static fh_entry_t *fh_list[64];

void
fh_init(void) {
    for (int i = 0; i < 64; ++i) {
        fh_list[i] = NULL;
    }
}

static uint64_t
fh_next(void) {
    for (int i = 0; i < 64; ++i) {
        if (fh_list[i] == NULL) {
            return i;
        }
    }
    return -1;
}

EP_STAT
fh_new(const char *path, fh_entry_t **fh) {
    *fh = calloc(sizeof(fh_entry_t), 1);
    (*fh)->fh = fh_next();
    if ((*fh)->fh == -1) {
        free(*fh);
        return EP_STAT_OUT_OF_MEMORY;
    }
    strcpy((*fh)->path, path);
    fh_list[(*fh)->fh] = *fh;
    return EP_STAT_OK;
}

void
fh_free(uint64_t fh) {
    if (fh_list[fh] == NULL) {
        return;
    }
    free(fh_list[fh]);
    fh_list[fh] = NULL;
}

// Returns the number of tokens. Path should be defined
// Assumes path begins with a /
size_t
split_path(const char *path, char ***tokens) {
    if (path == NULL || path[0] != '/') {
        ep_app_error("Does not support NULL path or relative paths. Path: %s",
                     path);
    }
    *tokens = calloc(sizeof(char *), 32);

    char c;
    size_t prev_index = 0;
    size_t index = 1;
    size_t tokens_index = 0;
    while ((c = path[index]) != '\0' && tokens_index < 32) {
        if (c == '/') {
            *tokens[tokens_index] = calloc(sizeof(char), 128);
            size_t length = index - prev_index - 1;
            strncpy(*tokens[tokens_index], path + prev_index,
                    length < 128 ? length : 127);
            tokens_index++;
            prev_index = index;
        }
        index++;
    }

    return tokens_index;
}

void
free_tokens(char **tokens) {
    if (tokens == NULL) {
        return;
    }
    for (int i = 0; i < 32; ++i) {
        if (tokens[i] == NULL) {
            break;
        }
        free(tokens[i]);
    }
    free(tokens);
}

void
get_human_name(const char *path, char human_name[256]) {
    strcpy(human_name, FILE_PREFIX);
    strcat(human_name, path);
}
