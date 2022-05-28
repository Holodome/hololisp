TARGET = hololisp
SRC_DIR = src
OUT_DIR = build
# If you want to compile for debugging, run 'make CFLAGS=-g'
CFLAGS = -O2

LOCAL_CFLAGS = -std=c89 -I$(SRC_DIR) -pedantic -Wshadow -Wextra -Wall -Werror
LOCAL_LDFLAGS = -pthread -lm

DEPFLAGS = -MT $@ -MMD -MP -MF $(OUT_DIR)/$*.d

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)

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

# Generate preprocessor output 
$(DIR)/%.i: $(SRC_DIR)/%.c
	$(CC) -E $(CFLAGS) -c -o $@ $<  

.PHONY: clean 
