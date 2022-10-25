CC=clang
ifeq ($(OS),Windows_NT)
	LIBEXT=dll
	PROGEXT=.exe
else
	UNAME:=$(shell uname -s)
	PROGEXT=
	ifeq ($(UNAME),Darwin)
		LIBEXT=dylib
	else ifeq ($(UNAME),Linux)
		LIBEXT=so
	else
		$(error OS not supported by this Makefile)
	endif
endif

default: lib

lib: ed2k.c
	$(CC) -shared -fpic -DED2K_LIBRARY ed2k.c -o libed2k.$(LIBEXT)

cli: ed2k.c
	$(CC) ed2k.c -o ed2k$(PROGEXT)

cli-threaded: ed2k.c
	$(CC) ed2k.c -DED2K_ENABLE_PTHREAD -pthread -o ed2k$(PROGEXT)

wasm: ed2k.c
	emcc -s WASM=1 ed2k.c -o ed2k.js

.PHONY: default lib cli cli-threaded wasm
