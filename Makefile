TARGET = build/hololisp
CFLAGS = -O2
LDFLAGS = -lm

$(shell mkdir -p build)

ifneq (,$(COV))
	COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
endif

LOCAL_CFLAGS = -std=c99 -Ihololisp -pedantic -Wshadow -Wextra -Wall -Werror

DEPFLAGS = -MT $@ -MMD -MP -MF build/$*.d

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

SRCS = hololisp/amalgamated.c
OBJS = $(SRCS:hololisp/%.c=build/%.o)

#
# Program rules
#

all: $(TARGET)

-include $(SRCS:hololisp/%.c=build/%.d)

$(TARGET): $(OBJS)
	$(CC) $(COVERAGE_FLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: hololisp/%.c
	$(CC) $(LOCAL_CFLAGS) $(DEPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -c -o $@ $<  

clean:
	rm -rf build 

#
# Test rules
#

TEST_DIR = tests
UNIT_TEST_OUT_DIR = build/$(TEST_DIR)
UNIT_TEST_PROJECT_SRCS = $(filter-out hololisp/main.c, $(SRCS))
UNIT_TEST_PROJECT_OBJS = $(UNIT_TEST_PROJECT_SRCS:hololisp/%.c=$(UNIT_TEST_OUT_DIR)/%.o) 
UNIT_TEST_SRCS = $(wildcard $(TEST_DIR)/*.c) 
UNIT_TESTS = $(UNIT_TEST_SRCS:$(TEST_DIR)/%.c=$(UNIT_TEST_OUT_DIR)/%.test)
	
UNIT_TEST_DEPFLAGS = -MT $@ -MMD -MP -MF $(UNIT_TEST_OUT_DIR)/$*.d

-include $(wildcard $(UNIT_TEST_OUT_DIR)/*.d)

$(UNIT_TEST_OUT_DIR)/%.test: $(UNIT_TEST_PROJECT_OBJS) $(UNIT_TEST_OUT_DIR)/%.o
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -g -O0 -o $@ $^ $(LDFLAGS)

$(UNIT_TEST_OUT_DIR)/%.o: hololisp/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) $(COVERAGE_FLAGS) -g -O0 -c -o $@ $<  

$(UNIT_TEST_OUT_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) -O0 -g -c -o $@ $<  

test tests: $(UNIT_TEST_OUT_DIR) $(UNIT_TESTS) all
	./scripts/test.sh

$(UNIT_TEST_OUT_DIR): build
	mkdir -p $(UNIT_TEST_OUT_DIR)

WASM_TARGET = build/hololisp.wasm
WASM_SOURCES = $(filter-out hololisp/main.c, $(SRCS))
WASM_FLAGS = -sSTRICT=1 -sALLOW_MEMORY_GROWTH=1 -sMALLOC=dlmalloc -sEXPORT_ES6=1
EMCC = emcc
EMRUN = emrun

wasm: build $(WASM_TARGET)

$(WASM_TARGET): $(WASM_SOURCES)
	$(EMCC) -O2 --no-entry -sEXPORTED_RUNTIME_METHODS=ccall,cwrap $(WASM_FLAGS) -o build/hololisp.js $^

wasm-test: $(SRCS)
	$(EMCC) -O0 --preload-file examples --emrun $(WASM_FLAGS) -o build/test.html $^
	./scripts/replace_arguments_emcc.py
	EXECUTABLE="emrun --browser firefox build/test.html --" ./tests/test_lisp.sh

FUZZ = build/fuzz

$(FUZZ): $(filter-out hololisp/main.c, $(SRCS)) tests/fuzz/fuzz.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -g -O0 -fsanitize=fuzzer,address $^ -o $@

fuzz: $(FUZZ)
	$(shell rm -r crash-*)

.PHONY: all test tests clean wasm-test fuzz
