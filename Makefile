# Makefile
# 28.5.2022 Holodome
# An attempt to Make kinda universal Makefile for C
#
# Notes:
# 1. Must have target directory separate from source directories (otherwise 'clean' would delete everything)
# 2. Source code directory must not have subdirectories
#
# Separate from that, all variables should be freely configurable

#
# Constants
#

TARGET = hololisp
SRC_DIR = src
OUT_DIR = build
# If you want to compile for debugging, run 'make CFLAGS=-g'
CFLAGS = -O2

ifneq (,$(COV))
	COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
endif 
# To separate CLI-settable parameters from constant. These are constant
LOCAL_CFLAGS = -std=c99 -I$(SRC_DIR) -pedantic -Wshadow -Wextra -Wall -Werror
LOCAL_LDFLAGS = -pthread -lm

# Flags used to generate .d files
DEPFLAGS = -MT $@ -MMD -MP -MF $(OUT_DIR)/$*.d

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
	$(CC) $(COVERAGE_FLAGS) -o $(OUT_DIR)/$@ $^ $(LOCAL_LDFLAGS) $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(LOCAL_CFLAGS) $(DEPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -c -o $@ $<  

run: $(TARGET) 
	$(OUT_DIR)/$(TARGET)

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
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -g -O0 -o $@ $^

$(UNIT_TEST_OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) $(COVERAGE_FLAGS) -g -O0 -c -o $@ $<  

$(UNIT_TEST_OUT_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) -O0 -g -c -o $@ $<  

test tests: $(UNIT_TEST_OUT_DIR) $(UNIT_TESTS) all
	for file in $(UNIT_TESTS) ; do echo "Running $$file" && $$file || exit 1 ; done 
	for file in tests/*.sh ; do echo "Running $$file" && $$file || exit 1 ; done

$(UNIT_TEST_OUT_DIR): $(OUT_DIR)
	mkdir -p $(UNIT_TEST_OUT_DIR)

.PHONY: all test clean 
