# Makefile
# 28.5.2022 Holodome
# An attempt to Make kinda universal Makefile for C
#
# Notes:
# 1. Must have target directory separate from source directories (otherwise 'clean' would delete everything)
# 2. Source code directory must not have subdirectories
# 3. check (https://libcheck.github.io/check/) is used for unit testing
#
# Separate from that, all variables should be freely configurable

#
# Constants
#

TARGET = hololisp
SRC_DIR = src
OUT_DIR = build
TEST_DIR = tests
# If you want to compile for debugging, run 'make CFLAGS=-g'
CFLAGS = -O2

# To separate CLI-settable parameters from constant. These are constant
LOCAL_CFLAGS = -std=c89 -I$(SRC_DIR) -pedantic -Wshadow -Wextra -Wall -Werror
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
	$(CC) -o $(OUT_DIR)/$@ $(OBJS) $(LOCAL_LDFLAGS) $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(LOCAL_CFLAGS) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<  

run: $(TARGET) 
	$(OUT_DIR)/$(TARGET)

clean:
	rm -rf $(OUT_DIR)

.PHONY: clean 

#
# Test rules
#

TESTS = $(wildcard $(TEST_DIR)/*.c) $(filter-out $(SRC_DIR)/main.c, $(SRCS))

# Note: We disable pedantic here because check does not goes along well with it (sigh)
unit_test: $(TESTS) 
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -Wno-pedantic -g -O0 -o $(OUT_DIR)/$@ -lcheck $(TESTS)

test tests check: all unit_test
	$(OUT_DIR)/unit_test

