SRC_DIRS     = ./src
TEST_DIRS    = ./tests ./src/sat
CFLAGS       = -MMD -MP -std=c2x -Wall -Wextra -Werror -pedantic -Werror=vla -Wno-unused-function
LDFLAGS      = -lm

FORMAT_FILES := $(shell find $(SRC_DIRS) $(TEST_DIRS) -path '$(SRC_DIRS)/util' -prune -o -type f \( -name '*.c' -o -name '*.h' \) -print)

ifeq ($(MAKECMDGOALS),debug)

	TARGET_EXEC  = Cluedo
	BUILD_DIR    =  ./bin/debug
	CFLAGS       += -DDEBUG -g3 -O0 -Wno-unused-variable -Wno-unused-but-set-parameter -Wno-unused-parameter -Wno-unused-but-set-variable
	LDFLAGS      += -lreadline
	SRCS = $(shell find $(SRC_DIRS) -name *.c)

else ifeq ($(MAKECMDGOALS),test)

	TARGET_EXEC  = sat_test
	BUILD_DIR    =  ./bin/test
	CFLAGS       += -DDEBUG -O2
	SRCS = $(shell find $(TEST_DIRS) -name *.c)

else

	TARGET_EXEC  = Cluedo
	BUILD_DIR    = ./bin/release
	CFLAGS       += -DNDEBUG -O2
	LDFLAGS      += -lreadline
	SRCS = $(shell find $(SRC_DIRS) -name *.c)

endif

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

all: $(BUILD_DIR)/$(TARGET_EXEC)

debug: $(BUILD_DIR)/$(TARGET_EXEC)

test: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo Done. Placed executable at $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean format format-check check install-hooks test
clean:
	$(RM) -r ./bin

format:
	@for f in $(FORMAT_FILES); do \
		if ! clang-format --style=file "$$f" | diff -q "$$f" - > /dev/null 2>&1; then \
			clang-format -i --style=file "$$f"; \
			echo "Reformatted: $$f"; \
		fi; \
	done

format-check:
	@for f in $(FORMAT_FILES); do \
		clang-format --dry-run --Werror --style=file "$$f"; \
	done

check:
	@cppcheck --enable=warning,style,performance,portability --std=c23 --language=c --error-exitcode=1 --quiet --suppress=memleakOnRealloc:src/sat/cdcl.c --suppress=memleakOnRealloc:tests/main.c -i src/util src tests

install-hooks:
	@git config core.hooksPath .githooks
	@echo "Configured git hooks path to .githooks"

-include $(DEPS)