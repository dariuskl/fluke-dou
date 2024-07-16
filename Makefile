CFLAGS += -Wall -Wextra -Werror -pedantic -pedantic-errors
CFLAGS += -std=c17
CFLAGS += -Os
CFLAGS += -mmcu=msp430g2452
LDFLAGS += -Tsrc/msp430g2452.ld -Wl,-print-memory-usage

.PHONY all: build/msp430g2452_1900a build/msp430g2452_8000a

build/msp430g2452_1900a: src/1900a_firmware.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map $< -o $@
	/opt/gcc-msp430-none/bin/msp430-elf-objdump -D $@ > $@.S
	/opt/gcc-msp430-none/bin/msp430-elf-objcopy -O binary $@ $@.bin

build/msp430g2452_8000a: src/8000a_firmware.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map $< -o $@
	/opt/gcc-msp430-none/bin/msp430-elf-objdump -D $@ > $@.S
	/opt/gcc-msp430-none/bin/msp430-elf-objcopy -O binary $@ $@.bin
