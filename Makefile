ERROR_POLICY =-pedantic -Wshadow -Wextra -Wall -Werror -Wno-c11-extensions
CFLAGS =-g -O0 -std=c99 -Isrc -D_CRT_SECURE_NO_WARNINGS $(ERROR_POLICY)

DIR = build
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:%.c=$(DIR)/%.o)
DEPS = $(wildcard src/*.h)

TESTS = $(wildcard tests/*.c)
TEST_EXES = $(TESTS:%.c=$(DIR)/%.exe)

all: hololisp 

hololisp: $(OBJS) | $(DIR) $(DEPS)
	$(CC) -o $(DIR)/$@ $^ $(LDFLAGS)

run: hololisp 
	$(DIR)/holoc

$(DIR)/%.i: %.c
	mkdir -p $(dir $@)
	$(CC) -E $(CFLAGS) -c -o $@ $<  

$(DIR)/%.o: %.c | $(DEPS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<  

$(DIR):
	mkdir -p $@

clean:
	rm -rf $(DIR)

.PHONY: clean
