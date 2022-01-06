cc = gcc
cflags = -Wall -g -pedantic -fsanitize=address -std=c99
build_dir = ./build
exec = $(build_dir)/csort
objs = core.o config.o csort.o

$(exec): main.c core.c config.c csort.c
	$(cc) $(cflags) $^ -o $@ ./external/lua/liblua54.so -lm

$(build_dir)/csort.o: csort.c
	$(cc) $(cflags) -c $^ -o $@

$(build_dir)/config.o: config.c
	$(cc) $(cflags) -c $^ -o $@

check: test/check.c core.c config.c csort.c
	$(cc) $(cflags) $^ -o $(build_dir)/check ./external/lua/liblua54.so -lm

debug: $(exec)
	gdb -q $(exec)

debug_test: $(build_dir)/check
	gdb -q $(build_dir)/check

clean:
	rm -rf build/*
