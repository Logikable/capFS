# CapFS

## Getting Started

Build: `make`

Mount: 
 * `make run`, or
 * `bin/capfs -f [mount point]`

Clean: `make clean`

Test: `make test TEST=[test_name].c`

Python tests (while the FS is mounted): `python3 src/test/[test_name].py`

## TODO

Highest to lowest priority:

1. Fix FUSE code so that we can at least run benchmarks on everything. Start with the tests in `src/test`, specifically `integration.c` and the Python tests. Please write more!!
2. Currently, all code is clientside. We want all GDP calls to be server-side (so that we can add Raft in the future), which requires writing a separate server. The client and server connect using protobuf.
3. Local caching needs to be performed on local state (see: `capfs_dir_table_t` in `src/capfs_dir.h` and `inode_t` in `src/capfs_file.h`), as well as data (indirect blocks and data blocks).
4. Log creation takes a long time (1-2 seconds). Precreate them in the background or on startup and save them for future use. Logs are identifiable by `gdp_name_t`, or GOBs in the literature. These are char[32] arrays, and can be stored (see `capfs_file_t` in `src/capfs_file.h`) and passed around. See how the GDPFS folks did their implementation of precreation [here](https://github.com/paulbramsen/gdpfs/blob/master/src/gdpfs_log.c).

## Useful links

* [GDP Wiki](https://gdp.cs.berkeley.edu/redmine/projects/gdp/wiki/GDP) - there's a bunch of random useful information here.
* [GDPFS src](https://github.com/paulbramsen/gdpfs/tree/master/src) - as a reference point.
* [FUSE information](https://libfuse.github.io/doxygen/structfuse__operations.html) - also as a reference.
* GDP API - find that in the GDP repository at `gdp/doc/developer/gdp-programmatic-api.html`. I also suggest using the source code in `gdp/gdp` as a reference point. The READMEs in the root directory of the repository also are a little bit useful.

## Code Organization

Imagine the stack as follows:

* FUSE
* capfs.c
* capfs_dir.c
* capfs_file.c
* GDP (+ potentially protobuf)

### Overview

File systems consist of directories and files. In CapFS, directories are files, storing an array of adjacent directory entries (`capfs_dir_entry_t` in `src/capfs_dir.h`) in the data of the file. Each file is a log in GDP. Each file starts at an inode, which contains direct and indirect pointers (to indirect tables). When a data block is written to by the user, a log record consisting of the 32KB data block containing the user's write and the updated inode is appended to the log. If an indirect table was modified, it is appended in the same log record. Reads are performed through a series of redirects: the last record is read for the most up-to-date inode, and the corresponding direct or indirect pointer is calculated. This pointer is actually a record number (`recno`), tracking the last edit of the data block (or indirect block) of interest. That record is then read, and the data is either retrieved, or in the case of an indirect block, a second pointer is calculated and record number accessed.

### capfs.c

FUSE exposes [a long list of operations](https://libfuse.github.io/doxygen/structfuse__operations.html), many of which are implemented in `src/capfs.c`. Note that website has many inaccuracies about function signatures. All functions here are inline (static) and are prefixed with `capfs_`. Perform tests by writing C code that make syscalls (e.g. `src/test/integration.c`), or Python code that makes file calls (e.g. `src/test/create.py`, `src/test/write.py`). Only do the latter if you are confident in the C tests!! Python makes a TON of random syscalls. This part is likely where most of the bugs are! Also, **there are some FUSE functions that are unimplemented but may be called!**

### capfs_dir.c

This is where directory logic is stored (anything that doesn't operate directly on a file). Since you are given string paths by FUSE, you need to translate them into `capfs_dir_t` objects to store and perform operations on directories in the future. `capfs_dir_t` contain a `capfs_file_t` pointer if you want to work on the underlying file. Tests are mostly written for this part (see `src/test`, and look for the file name corresponding to the function you want to test). This part is somewhat robust -- it has been mostly tested, but there are several tests missing.

### capfs_file.c

This is where file logic is stored. It is the most robust because it was written and tested first. It currently interfaces directly with the GDP wherever necessary. There are a ton of helper functions that perform grunt work of talking to the GDP, as well as external-facing functions that perform higher level operations (create, read, write, open, close).

### capfs_util.c

Utility functions for working with FUSE file handlers (they are just uint64_t numbers); they work similar to file descriptors in ext4 and PintOS. Also utility functions for parsing string paths into an array of strings. A utility function for converting human_name to a GDP human name is also in here, but is rarely used.
