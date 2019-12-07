.PHONY: default
default: build

build:
	@(cd capfs && $(MAKE) build)
clean:
	@(cd capfs && $(MAKE) clean)
