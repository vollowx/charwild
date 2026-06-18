BUILD_DIR := build
SRC_DIR   := src
TARGET    := $(BUILD_DIR)/charwild

SRC := $(shell find src -name '*.c') thirdparty/simplexnoise1234.c
OBJ := $(patsubst %.c,build/%.o,$(SRC))

CC       ?= gcc
CFLAGS   := -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-missing-field-initializers -I$(SRC_DIR) -Ithirdparty
DEPFLAGS := -MMD -MP
LDFLAGS  :=
LDLIBS   := -lncurses -lmenu

.PHONY: all all-release clean bear

all: CFLAGS += -O0 -g3 -fsanitize=address,undefined -DCW_USE_ASAN
all: LDFLAGS += -fsanitize=address,undefined
all: $(TARGET)

all-release: CFLAGS += -O3 -DNDEBUG
all-release: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(DEPFLAGS) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

bear:
	rm -rf $(BUILD_DIR)
	bear --output /tmp/charwild_compile_commands.json -- make all
	@mkdir -p $(BUILD_DIR)
	mv /tmp/charwild_compile_commands.json $(BUILD_DIR)/compile_commands.json

-include $(OBJ:.o=.d)
