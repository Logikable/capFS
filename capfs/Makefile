.PHONY: default
default: build

CC = gcc
WALL = -Wall
DEBUG = -O0 -g -fvar-tracking
FUSE_LIBS=`pkg-config fuse --cflags --libs`

CFLAGS = $(WALL) $(FUSE_LIBS) $(DEBUG)
LFLAGS = $(CFLAGS) -lgdp -lep

BINDIR = ../bin
BINOUT = capfs
BUILDDIR = build
SOURCES = $(wildcard *.c)
OBJ = $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))

build: $(BINDIR)/$(BINOUT)

$(BINDIR)/$(BINOUT): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LFLAGS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BINDIR)
	rm -rf $(BUILDDIR)
