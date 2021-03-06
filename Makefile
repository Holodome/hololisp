SRC_DIR = hololisp
OUT_DIR = build
TARGET = $(OUT_DIR)/hololisp
LIB = $(OUT_DIR)/hololisp.a
CFLAGS = -O2

ifneq (,$(COV))
	COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
endif 
# To separate CLI-settable parameters from constant. These are constant
LOCAL_CFLAGS = -std=c99 -I$(SRC_DIR) -pedantic -Wshadow -Wextra -Wall -Werror
LOCAL_LDFLAGS = -pthread -lm

DEPFLAGS = -MT $@ -MMD -MP -MF $(OUT_DIR)/$*.d

ifneq (,$(DEBUG))
	CFLAGS=-g
endif 

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
OBJS_BUT_MAIN = $(filter-out $(OUT_DIR)/main.o, $(OBJS))

#
# Program rules
#

all: $(OUT_DIR) $(TARGET) tools

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

-include $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.d)

$(TARGET): $(OBJS) 
	$(CC) $(COVERAGE_FLAGS) -o $@ $^ $(LOCAL_LDFLAGS) $(LDFLAGS)

$(LIB): $(OBJS_BUT_MAIN)
	ar rcs $@ $^

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
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -g -O0 -o $@ $^

$(UNIT_TEST_OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) $(COVERAGE_FLAGS) -g -O0 -c -o $@ $<  

$(UNIT_TEST_OUT_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(UNIT_TEST_DEPFLAGS) -O0 -g -c -o $@ $<  

test tests: $(UNIT_TEST_OUT_DIR) $(UNIT_TESTS) all
	./scripts/test.sh

$(UNIT_TEST_OUT_DIR): $(OUT_DIR)
	mkdir -p $(UNIT_TEST_OUT_DIR)


FORMATTER = $(OUT_DIR)/hololisp-format
FORMATTER_DIR = tools/formatter
FORMATTER_SRCS = $(wildcard $(FORMATTER_DIR)/*.c)

$(FORMATTER): $(FORMATTER_SRCS) $(LIB)
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -o $@ $^

tools: $(FORMATTER)

.PHONY: all test clean tools
