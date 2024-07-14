CFLAGS += -Wall -Wextra -Werror -pedantic -pedantic-errors -std=c17
LDFLAGS += -Tsrc/msp430g2452.ld

.PHONY all: build/msp430g2452_1900a

build/msp430g2452_1900a: src/1900a_firmware.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@
