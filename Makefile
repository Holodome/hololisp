EMCC = emcc
EMRUN = emrun

SRC_DIR = hololisp
OUT_DIR = build
TARGET = $(OUT_DIR)/hololisp
CFLAGS = -O2
LDFLAGS = -lm

ifneq (,$(COV))
	COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
endif 
# To separate CLI-settable parameters from constant. These are constant
LOCAL_CFLAGS = -std=c99 -I$(SRC_DIR) -pedantic -Wshadow -Wextra -Wall -Werror 

DEPFLAGS = -MT $@ -MMD -MP -MF $(OUT_DIR)/$*.d

WASM_FLAGS := -sSTRICT=1 -sALLOW_MEMORY_GROWTH=1 -sMALLOC=dlmalloc -sMODULARIZE=1 -sEXPORT_ES6=1 

ifneq (,$(DEBUG))
	CFLAGS+=-g -DHLL_DEBUG -DHLL_MEM_CHECK -DHLL_STRESS_GC -fsanitize=address
#	CFLAGS+=-g -DHLL_DEBUG -DHLL_MEM_CHECK -fsanitize=address
	LDFLAGS+= -fsanitize=address
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)

#
# Program rules
#

all: $(OUT_DIR) $(TARGET)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

-include $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.d)

$(TARGET): $(OBJS)
	$(CC) $(COVERAGE_FLAGS) -o $@ $^ $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(DEPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -c -o $@ $<  

clean:
	rm -rf $(OUT_DIR) 

#
# Test rules
#

TEST_DIR = tests
UNIT_TEST_OUT_DIR = $(OUT_DIR)/$(TEST_DIR)
UNIT_TEST_PROJECT_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))
UNIT_TEST_PROJECT_OBJS = $(UNIT_TEST_PROJECT_SRCS:$(SRC_DIR)/%.c=$(UNIT_TEST_OUT_DIR)/%.o) 
UNIT_TEST_SRCS = $(wildcard $(TEST_DIR)/*.c) 
UNIT_TESTS = $(UNIT_TEST_SRCS:$(TEST_DIR)/%.c=$(UNIT_TEST_OUT_DIR)/%.test)
	
UNIT_TEST_DEPFLAGS = -MT $@ -MMD -MP -MF $(UNIT_TEST_OUT_DIR)/$*.d

-include $(wildcard $(UNIT_TEST_OUT_DIR)/*.d)

$(UNIT_TEST_OUT_DIR)/%.test: $(UNIT_TEST_PROJECT_OBJS) $(UNIT_TEST_OUT_DIR)/%.o
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -g -O0 -o $@ $^ $(LDFLAGS)

$(UNIT_TEST_OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) $(COVERAGE_FLAGS) -g -O0 -c -o $@ $<  

$(UNIT_TEST_OUT_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) -O0 -g -c -o $@ $<  

test tests: $(UNIT_TEST_OUT_DIR) $(UNIT_TESTS) all
	./scripts/test.sh

$(UNIT_TEST_OUT_DIR): $(OUT_DIR)
	mkdir -p $(UNIT_TEST_OUT_DIR)

WASM_TARGET = $(OUT_DIR)/hololisp.wasm
WASM_SOURCES = $(filter-out $(SRC_DIR)/main.c, $(SRCS))

wasm: $(OUT_DIR) $(WASM_TARGET)

$(WASM_TARGET): $(WASM_SOURCES)
	$(EMCC) -Os --no-entry -sEXPORTED_RUNTIME_METHODS=ccall,cwrap $(WASM_FLAGS) -o $(OUT_DIR)/hololisp.js $^

wasm-test: $(SRCS)
	$(EMCC) -O0 --preload-file examples --emrun $(WASM_FLAGS) -o $(OUT_DIR)/test.html $^
	./scripts/replace_arguments_emcc.py
	EXECUTABLE="emrun --browser firefox build/test.html --" ./tests/test_lisp.sh


.PHONY: all test tests clean wasm-test
