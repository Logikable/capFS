.PHONY: default
.PRECIOUS: %.pb-c.c
default: build

CC = gcc
WALL = -Wall
DEBUG = -O0 -g -fvar-tracking
FUSE_LIBS = `pkg-config fuse --cflags --libs`

CFLAGS = $(WALL) $(FUSE_LIBS) $(DEBUG) -I .
LFLAGS = $(CFLAGS) -lgdp -lep -lprotobuf-c

EXT = c

PROTODIR = proto
PROTOSRCS = $(wildcard $(PROTODIR)/*.proto)
PROTOFLAGS = --c_out=.

BINDIR = ../bin
BINOUT = capfs
BUILDDIR = build
SOURCES = $(patsubst %.proto, %.pb-c.c, $(PROTOSRCS)) $(wildcard capfs*.$(EXT))
OBJ = $(patsubst %.$(EXT), $(BUILDDIR)/%.o, $(SOURCES))

MNTPOINT = /mnt/capfs
MNTFLAGS = -f $(MNTPOINT)

TEST = gdp.c
TESTDIR = test
TESTOUT = capfs_test
TESTOBJ = $(patsubst %.$(EXT), $(BUILDDIR)/%.o, $(TEST))

build: $(BINDIR)/$(BINOUT)

$(BINDIR)/$(BINOUT): $(OBJ) $(BUILDDIR)/main.o
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LFLAGS)

$(BUILDDIR)/%.o: %.$(EXT)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

%.pb-c.c: %.proto
	protoc-c $< $(PROTOFLAGS)

clean:
	rm -rf $(BINDIR)
	rm -rf $(BUILDDIR)
	rm -rf $(wildcard $(PROTODIR)/*.$(EXT)) $(wildcard $(PROTODIR)/*.h)
	sudo umount $(MNTPOINT) || /bin/true
	sudo rm -rf $(MNTPOINT) || sudo rmdir $(MNTPOINT) || /bin/true

run: build
	sudo mkdir -p $(MNTPOINT)
	sudo chmod 447 $(MNTPOINT)
	$(BINDIR)/$(BINOUT) $(MNTFLAGS)

test: $(BINDIR)/$(TESTOUT)
	$(BINDIR)/$(TESTOUT)

$(BINDIR)/$(TESTOUT): $(OBJ) $(TESTOBJ)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LFLAGS)

$(BUILDDIR)/%.o: $(TESTDIR)/%.$(EXT)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
