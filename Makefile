CC=cc -Iinclude/ -I/usr/include/ --std=c11
CPP=c++ -Iinclude/ -I/usr/include/ --std=c++14
CFLAGS_DEBUG=-Wall -Werror -pedantic -Og -g
CFLAGS_RELEASE=-Wall -Werror -pedantic -O3 -march=native
CFLAGS=$(CFLAGS_RELEASE)
AS=as -msyntax=intel
SRCS=src/kozet_coroutine/kcr.c src/kozet_coroutine/internal/x64/stackinit.c
ASMS=src/kozet_coroutine/internal/x64/switch.s
BUILD_DIR=build
BUILD_TEMP_DIR=build/objs
OBJS=$(addprefix $(BUILD_TEMP_DIR)/,$(SRCS:.c=.o))
ASMOBJS=$(addprefix $(BUILD_TEMP_DIR)/,$(ASMS:.s=.o))

all: bestos

bestos: $(BUILD_DIR)/test $(BUILD_DIR)/test_cpp $(BUILD_DIR)/test_benchmark

$(BUILD_DIR)/test: test/main.c \
		$(BUILD_DIR)/libkozet_coroutine.a \
		include/kozet_coroutine/kcr.h
	@mkdir -p $(BUILD_DIR)
	@echo -e '\e[33mCompiling $(BUILD_DIR)/test...\e[0m'
	@$(CC) test/main.c $(BUILD_DIR)/libkozet_coroutine.a \
		-o $(BUILD_DIR)/test $(CFLAGS)
	@echo -e '\e[32mDone!\e[0m'

$(BUILD_DIR)/test_benchmark: test/benchmark.c \
		$(BUILD_DIR)/libkozet_coroutine.a \
		include/kozet_coroutine/kcr.h
	@mkdir -p $(BUILD_DIR)
	@echo -e '\e[33mCompiling $(BUILD_DIR)/test_benchmark...\e[0m'
	@$(CC) test/benchmark.c $(BUILD_DIR)/libkozet_coroutine.a \
		-o $(BUILD_DIR)/test_benchmark $(CFLAGS)
	@echo -e '\e[32mDone!\e[0m'

$(BUILD_DIR)/test_cpp: test/main.cpp \
		$(BUILD_DIR)/libkozet_coroutine.a \
		include/kozet_coroutine/kcr.h
	@mkdir -p $(BUILD_DIR)
	@echo -e '\e[33mCompiling $(BUILD_DIR)/test_cpp...\e[0m'
	@$(CPP) test/main.cpp $(BUILD_DIR)/libkozet_coroutine.a -o $(BUILD_DIR)/test_cpp $(CFLAGS)
	@echo -e '\e[32mDone!\e[0m'

$(BUILD_DIR)/libkozet_coroutine.a: $(OBJS) $(ASMOBJS)
	@mkdir -p $(BUILD_DIR)
	@echo -e '\e[33mLinking $@...\e[0m'
	@$(AR) rcs $@ $^

$(BUILD_TEMP_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo -e '\e[33mAssembling $@...\e[0m'
	@$(AS) $< -o $@

$(BUILD_TEMP_DIR)/%.o: %.c $(BUILD_TEMP_DIR)/%.d
	@mkdir -p $(dir $@)
	@echo -e '\e[33mCompiling $@...\e[0m'
	@$(CC) -c $(CFLAGS) $< -o $@

DEPS:=$(OBJS:.o=.d)

$(BUILD_TEMP_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM $< | \
		sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' > $@

-include $(DEPS)

clean:
	@echo -e '\e[31mCleaning up...\e[0m'
	@find . -type f -name "*.[do]" -delete
	@rm -rf $(BUILD_DIR)/
