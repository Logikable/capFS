.PHONY: default
default: build

SRCDIR = src

build:
	@(cd $(SRCDIR) && $(MAKE) build)
clean:
	@(cd $(SRCDIR) && $(MAKE) clean)
run:
	@(cd $(SRCDIR) && $(MAKE) run)
