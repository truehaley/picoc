# Top-level makefile loosely based on https://github.com/embeddedartistry/cmake-project-skeleton/blob/main/Makefile
# you can set this to 1 to see all commands that are being run
VERBOSE ?= 0

ifeq ($(VERBOSE),1)
export Q :=
export VERBOSE := 1
else
export Q := @
export VERBOSE := 0
endif

all: cmake build

######
## cmake commands
.PHONY: cmake
cmake:
	$(Q) cmake --preset default

build/rel/Makefile:
	$(Q) echo "PicoC has not been configured, please run 'make cmake' first"
	$(Q) exit 1


######
## build commands
.PHONY: build
build: build/rel/Makefile
	$(Q) make -C build/rel picoc

build/rel/picoc:
	$(Q) echo "PicoC has not been built, please run 'make build' first"
	$(Q) exit 1

######
## test commands
.PHONY: test
test: build/rel/picoc
	$(Q) make -C build/rel test

######
## clean commands
.PHONY: clean
clean: build/rel/Makefile
	$(Q) make -C build/rel clean

######
## obliterate commands
.PHONY: confirm-obliterate obliterate
confirm-obliterate:
		$(Q) echo "This will remove the entire selected build structure!"
		$(Q) echo "Are you sure? [y/N]" && read ans && [ $${ans:-N} = y ]

obliterate: confirm-obliterate
	$(Q) rm -rf build/rel



# count:
	# @echo "Core:"
	# @cat picoc.h interpreter.h picoc.c table.c lex.c parse.c expression.c platform.c heap.c type.c variable.c include.c debug.c | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	# @echo ""
	# @echo "Everything:"
	# @cat $(SRCS) *.h */*.h | wc
