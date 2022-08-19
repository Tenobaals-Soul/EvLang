CC := gcc

CFLAGS := -O3 -Wall -Wextra
OFLAGS := -O3 -Wall -Wextra
LDFLAGS :=

EXEC := evlc
BUILD_DIR := out/compiler
SRC_DIR := compiler/src

DEBUG_FLAGS = -DDEBUG -g


all: remove_executables $(EXEC)

remove_executables:
	@rm -f $(EXEC)

set_debug: 
	$(eval CFLAGS += $(DEBUG_FLAGS))

debug: set_debug all

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:$(SRC_DIR)/%=$(BUILD_DIR)/%.o)

INC_DIRS=compiler/header
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OFLAGS) $(DEBUG_FLAGS) $(INC_FLAGS) -c $< -o $@

.PHONY: clean debug

clean:
	rm -f -r $(BUILD_DIR)/*
	rm -f $(EXEC)