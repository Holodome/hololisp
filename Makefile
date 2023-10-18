CFLAGS = -O2
LDFLAGS = -lm

$(shell mkdir -p build)

ifneq (,$(COV))
	COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
endif

DEPFLAGS = -MT $@ -MMD -MP -MF build/$*.d
CFLAGS += -std=c99 -Ihololisp -pedantic -Wshadow -Wextra -Wall -Werror -Wno-gnu-label-as-value

ifneq (,$(ASAN))
	CFLAGS += -fsanitize=address 
	LDFLAGS += -fsanitize=address
endif

ifneq (,$(STRESS_GC))
	CFLAGS += -DHLL_STRESS_GC
endif

ifneq (,$(DEBUG))
	CFLAGS += -O0 -g -DHLL_DEBUG
else 
	CFLAGS += -DNDEBUG
endif

SRCS = hololisp/main.c
# SRCS = hololisp/hll_builtins.c \
# 	hololisp/hll_bytecode.c \
# 	hololisp/hll_compiler.c \
# 	hololisp/hll_gc.c \
# 	hololisp/hll_mem.c \
# 	hololisp/hll_meta.c \
# 	hololisp/hll_value.c \
# 	hololisp/hll_vm.c \
# 	hololisp/main.c

#
# Program rules
#

all: build/hololisp

-include $(SRCS:hololisp/%.c=build/%.d)

build/hololisp: $(SRCS:hololisp/%.c=build/%.o)
	$(CC) $(COVERAGE_FLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: hololisp/%.c
	$(CC) $(DEPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -c -o $@ $<  

clean:
	rm -rf build 

format:
	$(Q)find hololisp \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;

#
# Test rules
#

UNIT_TEST_OUT_DIR = build/tests
UNIT_TEST_PROJECT_SRCS = hololisp/amalgamated.c
UNIT_TEST_PROJECT_OBJS = $(UNIT_TEST_PROJECT_SRCS:hololisp/%.c=$(UNIT_TEST_OUT_DIR)/%.o) 
UNIT_TEST_SRCS = $(wildcard tests/*.c) 
UNIT_TESTS = $(UNIT_TEST_SRCS:tests/%.c=$(UNIT_TEST_OUT_DIR)/%.test)
	
UNIT_TEST_DEPFLAGS = -MT $@ -MMD -MP -MF $(UNIT_TEST_OUT_DIR)/$*.d

-include $(wildcard $(UNIT_TEST_OUT_DIR)/*.d)

$(UNIT_TEST_OUT_DIR)/%.test: $(UNIT_TEST_PROJECT_OBJS) $(UNIT_TEST_OUT_DIR)/%.o
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -g -O0 -o $@ $^ $(LDFLAGS)

$(UNIT_TEST_OUT_DIR)/%.o: hololisp/%.c
	$(CC) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) $(COVERAGE_FLAGS) -g -O0 -c -o $@ $<  

$(UNIT_TEST_OUT_DIR)/%.o: tests/%.c
	$(CC) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) -O0 -g -c -o $@ $<  

test tests: $(UNIT_TEST_OUT_DIR) $(UNIT_TESTS) all
	./scripts/test.sh

$(UNIT_TEST_OUT_DIR): build
	mkdir -p $(UNIT_TEST_OUT_DIR)

WASM_FLAGS = -sSTRICT=1 -sALLOW_MEMORY_GROWTH=1 -sMALLOC=dlmalloc -sEXPORT_ES6=1
EMCC = emcc
EMRUN = emrun

wasm: build/hololisp.wasm

build/hololisp.wasm: hololisp/amalgamated.c
	$(EMCC) -O2 --no-entry -sEXPORTED_RUNTIME_METHODS=ccall,cwrap $(WASM_FLAGS) -o build/hololisp.js $^

wasm-test: $(SRCS)
	$(EMCC) -O0 --preload-file examples --emrun $(WASM_FLAGS) -o build/test.html $^
	./scripts/replace_arguments_emcc.py
	EXECUTABLE="emrun --browser firefox build/test.html --" ./tests/test_lisp.sh

FUZZ = build/fuzz

$(FUZZ): hololisp/amalgamated.c tests/fuzz/fuzz.c
	$(CC) $(CFLAGS) -g -O0 -fsanitize=fuzzer,address $^ -o $@

fuzz: $(FUZZ)
	$(shell rm -r crash-*)

.PHONY: all test tests clean wasm-test fuzz wasm
