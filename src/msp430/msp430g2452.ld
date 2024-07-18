MEMORY {
    ram (rw) : ORIGIN = 0x0200, LENGTH = 256
    info (r) : ORIGIN = 0x1000, LENGTH = 256
    rom (rx) : ORIGIN = 0xe000, LENGTH = 8192 - 64
    vectors : ORIGIN = 0xffc0, LENGTH = 64
}

INCLUDE "msp430g2xx.ld"
