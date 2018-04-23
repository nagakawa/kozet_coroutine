CC=cc -Iinclude/ -I/usr/include/ --std=c11
CPP=c++ -Iinclude/ -I/usr/include/ --std=c++14
CFLAGS=-Wall -Werror -pedantic -Og -g
CFLAGS_RELEASE=-Wall -Werror -pedantic -O3 -march=native
AS=as -msyntax=intel
SRCS=src/kozet_coroutine/kcr.c src/kozet_coroutine/internal/x64/stackinit.c
ASMS=src/kozet_coroutine/internal/x64/switch.s
OBJS=$(SRCS:.c=.o)
ASMOBJS=$(ASMS:.s=.o)

all: build/test

build/test: test/main.c \
		build/libkozet_coroutine.a \
		include/kozet_coroutine/kcr.h
	@mkdir -p build
	@echo -e '\e[33mCompiling test program...\e[0m'
	@$(CC) test/main.c build/libkozet_coroutine.a -o build/test $(CFLAGS)
	@echo -e '\e[32mDone!\e[0m'

build/libkozet_coroutine.a: $(OBJS) $(ASMOBJS)
	@mkdir -p build
	@echo -e '\e[33mLinking $@...\e[0m'
	@ar rcs $@ $^

%.o: %.s
	@echo -e '\e[33mAssembling $@...\e[0m'
	@$(AS) $< -o $@

%.o: %.c %.d
	@echo -e '\e[33mCompiling $@...\e[0m'
	@$(CC) -c $(CFLAGS) $< -o $@

DEPS:=$(OBJS:.o=.d)

%.d: %.c
	@$(CC) $(CFLAGS) -MM $< | \
		sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' > $@

-include $(DEPS)

clean:
	@echo -e '\e[31mCleaning up...\e[0m'
	@find . -type f -name "*.[do]" -delete
	@rm -rf build/
