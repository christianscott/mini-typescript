default: all

all: build test

build:
	bazel build //...

test:
	@echo "no tests specified"
	@exit 1
