CFLAGS += -Wall -Wextra -pedantic -Wno-unused-function -Wconversion -Wsign-conversion
CFLAGS += -std=c17
CFLAGS += -Os
LDFLAGS += -Lsrc/msp430 -Wl,-print-memory-usage

.PHONY all: build/msp430g2452_1900a \
			build/msp430g2231_8000a \
			build/msp430g2231_info_util \
			build/tlv_test

build/msp430g2452_1900a: src/1900a_firmware.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) -mmcu=msp430g2452 $(LDFLAGS) -Tmsp430g2452.ld -Wl,-Map,$@.map $< -o $@
	/opt/gcc-msp430-none/bin/msp430-elf-objdump -D $@ > $@.S
	/opt/gcc-msp430-none/bin/msp430-elf-objcopy -O binary $@ $@.bin

build/msp430g2231_8000a: src/8000a_firmware.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) -mmcu=msp430g2231 $(LDFLAGS) -Tmsp430g2231.ld -Wl,-Map,$@.map $< -o $@
	/opt/gcc-msp430-none/bin/msp430-elf-objdump -D $@ > $@.S
	/opt/gcc-msp430-none/bin/msp430-elf-objcopy -O binary $@ $@.bin

build/msp430g2231_info_util: src/msp430/info_util.c
	/opt/gcc-msp430-none/bin/msp430-elf-gcc $(CPPFLAGS) $(CFLAGS) -mmcu=msp430g2231 $(LDFLAGS) -Tmsp430g2231.ld -Wl,-Map,$@.map $< -o $@
	/opt/gcc-msp430-none/bin/msp430-elf-objdump -D $@ > $@.S
	/opt/gcc-msp430-none/bin/msp430-elf-objcopy -O binary $@ $@.bin

build/unity.o: lib/unity/unity.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Ilib/unity $(LDFLAGS) -c $^ -o $@

build/tlv_test: src/msp430/tlv_test.c build/unity.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -Ilib/unity $(LDFLAGS) $^ -o $@
	./$@
