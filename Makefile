CC = gcc
CFLAGS = -Wall -g -pedantic -fsanitize=address -std=c99
BUILD_DIR = ./build
EXEC = $(BUILD_DIR)/csort

run: $(EXEC)
	$(EXEC)

run_test: test
	$(BUILD_DIR)/test

debug: $(EXEC)
	gdb -q $(EXEC)

$(EXEC): main.c core.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/core.o: core.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf build
