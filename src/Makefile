.PHONY: default
.PRECIOUS: %.pb-c.c
default: build

CC = gcc
WALL = -Wall
DEBUG = -O0 -g -fvar-tracking
FUSE_LIBS=`pkg-config fuse --cflags --libs`

CFLAGS = $(WALL) $(FUSE_LIBS) $(DEBUG) -I .
LFLAGS = $(CFLAGS) -lgdp -lep -lprotobuf-c

EXT = c

PROTODIR = proto
PROTOSRCS = $(wildcard $(PROTODIR)/*.proto)
PROTOFLAGS = --c_out=.

BINDIR = ../bin
BINOUT = capfs
BUILDDIR = build
SOURCES = $(wildcard *.$(EXT)) $(patsubst %.proto, %.pb-c.c, $(PROTOSRCS))
OBJ = $(patsubst %.$(EXT), $(BUILDDIR)/%.o, $(SOURCES))

MNTPOINT = /mnt/capfs
MNTFLAGS = -f $(MNTPOINT)

build: $(BINDIR)/$(BINOUT)

$(BINDIR)/$(BINOUT): $(OBJ)
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
