CC = gcc
CFLAGS = -Wall -g -pedantic -fsanitize=address -std=c99
RELEASE_CFLAGS = -std=c99 -O2
BUILD_DIR = ./build
CHECK_BIN = $(BUILD_DIR)/check
BUILD_RELEASE = $(BUILD_DIR)/csort-release
EXEC = $(BUILD_DIR)/csort

run: $(EXEC)
	$(EXEC)

run_test: check
	$(CHECK_BIN)

debug: $(EXEC)
	gdb -q $(EXEC)

debug_test: $(CHECK_BIN)
	gdb -q $(CHECK_BIN)

release: main.c core.c config.c csort.c
	$(CC) $(RELEASE_CFLAGS) $^ -o $(BUILD_RELEASE) ./external/lua/liblua54.so -lm

$(EXEC): main.c core.c config.c csort.c
	$(CC) $(CFLAGS) $^ -o $@ ./external/lua/liblua54.so -lm

$(BUILD_DIR)/csort.o: csort.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(BUILD_DIR)/config.o: config.c
	$(CC) $(CFLAGS) -c $^ -o $@

check: test/check.c core.c config.c csort.c
	$(CC) $(CFLAGS) $^ -o $(CHECK_BIN) ./external/lua/liblua54.so -lm

$(BUILD_DIR)/core.o: core.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf build
