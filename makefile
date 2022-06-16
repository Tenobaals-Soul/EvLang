CC := gcc

CFLAGS := -O3 -Wall -Wextra
LDFLAGS :=

COMPILER_EXEC := evlc
COMPILER_BUILD_DIR := out/compiler
COMPILER_SRC_DIR := compiler/src

RUNTIME_EXEC := evlr
RUNTIME_BUILD_DIR := out/runtime
RUNTIME_SRC_DIR := runtime/src

DEBUG_FLAGS = -DDEBUG -g


all: remove_executables $(COMPILER_EXEC) $(RUNTIME_EXEC)

remove_executables:
	@rm -f $(COMPILER_EXEC)
	@rm -f $(RUNTIME_EXEC)
set_debug: $(eval CFLAGS += $(DEBUG_FLAGS))

debug: set_debug all

COMPILER_SRCS := $(shell find $(COMPILER_SRC_DIR) -name '*.c')
COMPILER_OBJS := $(COMPILER_SRCS:$(COMPILER_SRC_DIR)/%=$(COMPILER_BUILD_DIR)/%.o)

RUNTIME_SRCS := $(shell find $(RUNTIME_SRC_DIR) -name '*.c')
RUNTIME_OBJS := $(RUNTIME_SRCS:$(RUNTIME_SRC_DIR)/%=$(RUNTIME_BUILD_DIR)/%.o)

COMPILER_INC_DIRS=compiler/header
COMPILER_INC_FLAGS := $(addprefix -I,$(COMPILER_INC_DIRS))

RUNTIME_INC_DIRS=runtime/header
RUNTIME_INC_FLAGS := $(addprefix -I,$(RUNTIME_INC_DIRS))

$(COMPILER_EXEC): $(COMPILER_OBJS)
	$(CC) $(CFLAGS) $(COMPILER_OBJS) -o $@ $(LDFLAGS)

$(RUNTIME_EXEC): $(RUNTIME_OBJS)
	$(CC) $(CFLAGS) $(RUNTIME_OBJS) -o $@ $(LDFLAGS)

$(COMPILER_BUILD_DIR)/%.c.o: $(COMPILER_SRC_DIR)/%.c
	@mkdir -p $(COMPILER_BUILD_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(COMPILER_INC_FLAGS) -c $< -o $@

$(RUNTIME_BUILD_DIR)/%.c.o: $(RUNTIME_SRC_DIR)/%.c
	@mkdir -p $(RUNTIME_BUILD_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(RUNTIME_INC_FLAGS) -c $< -o $@

.PHONY: clean debug

clean:
	rm -f -r $(COMPILER_BUILD_DIR)/*
	rm -f $(COMPILER_EXEC)
	rm -f -r $(RUNTIME_BUILD_DIR)/*
	rm -f $(RUNTIME_EXEC)