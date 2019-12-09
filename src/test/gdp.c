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

#define STR(s) _STR(s)
#define _STR(s) #s
#define ITER 7

#define OK(ep) assert(EP_STAT_ISOK(ep))

#include <assert.h>
#include <string.h>

#include <ep/ep.h>
#include <gdp/gdp.h>

#include "capfs.h"
#include "capfs_file.h"

gdp_gin_t *test_create(void) {
    gdp_create_info_t *gci = gdp_create_info_new();
    OK(gdp_create_info_set_creator(
        gci, "CapFS", "fa19.cs262.eecs.berkeley.edu"));

    char human_name[256];
    strcpy(human_name, FILE_PREFIX);
    strcat(human_name, "test" STR(ITER));

    gdp_gin_t *ginp;
    OK(gdp_gin_create(gci, human_name, &ginp));

    return ginp;
}

gdp_gin_t *test_open(void) {
    gdp_open_info_t *goi = gdp_open_info_new();

    char human_name[256];
    strcpy(human_name, FILE_PREFIX);
    strcat(human_name, "test" STR(ITER));

    gdp_name_t gob;
    OK(gdp_parse_name(human_name, gob));

    gdp_gin_t *ginp;
    OK(gdp_gin_open(gob, GDP_MODE_RO, goi, &ginp));
    return ginp;
}

int main(int argc, char *argv[]) {
    init();

    gdp_gin_t *ginp = test_open();
    // gdp_gin_t *ginp = test_create();

    gdp_datum_t *datum = gdp_datum_new();
    OK(gdp_gin_read_by_recno(ginp, 0, datum));

    gdp_buf_t *dbuf = gdp_datum_getbuf(datum);
    size_t len = gdp_buf_getlength(dbuf);
    printf("%ld\n", len);

    char buf[256];
    gdp_buf_read(dbuf, (void *) buf, 256);

    for (int i = 0; i < 256; i++)
    {
        printf("%02X", buf[i]);
    }
    printf("\n");
}
