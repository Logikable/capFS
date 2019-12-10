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

#include <assert.h>
#include <sys/time.h>

#include <ep/ep.h>

#define OK(ep) assert(EP_STAT_ISOK(ep))
#define PRINT_ESTAT(ep, buf, sz) printf("%s\n", ep_stat_tostr(ep, buf, sz));

static long start;

void
bench_start(void) {
    struct timeval timecheck;

    gettimeofday(&timecheck, NULL);
    start = (long) (timecheck.tv_sec * 1000 + timecheck.tv_usec / 1000);
}

void
bench_end(void) {
    struct timeval timecheck;

    gettimeofday(&timecheck, NULL);
    long stop = (long) (timecheck.tv_sec * 1000 + timecheck.tv_usec / 1000);
    printf("Time taken: %ldms\n", stop - start);
}
