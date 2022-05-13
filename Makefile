ERROR_POLICY =-pedantic -Wshadow -Wextra -Wall -Werror
CFLAGS =-g -O0 -std=c89 -Isrc -D_CRT_SECURE_NO_WARNINGS $(ERROR_POLICY)

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

test: $(TEST_EXES) | $(DIR) $(DEPS) $(SRCS)
	@echo Running unit tests:
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done


$(DIR)/tests/%.exe: tests/%.c $(DEPS) $(SRCS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

$(DIR):
	mkdir -p $@

clean:
	rm -rf $(DIR)

.PHONY: clean test
