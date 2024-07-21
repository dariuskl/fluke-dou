// A utility program that writes the information memory.
// Meant to be used with the MSP-EXP430G2 Launchpad.

#include "g2231.c" // TODO include the suitable header
#include "tlv.c"

__attribute__((section(".info"))) static volatile struct {
  uint16_t segment_d[32];
  uint16_t segment_c[32];
  uint16_t segment_b[32];
  uint16_t segment_a[32];
} info;

_Static_assert(sizeof info == 256U, "info memory is 256 bytes");

static void erase_segment_a(void) {
  FCTL3 = FLASH_KEY | FCTL3_LOCKA; // unlock info memory
  FCTL1 = FLASH_KEY | FCTL1_ERASE;
  info.segment_a[16] = 0;
  FCTL1 = FLASH_KEY;
  FCTL3 = FLASH_KEY | FCTL3_LOCKA | FCTL3_LOCK; // lock info memory
}

static void write_segment_a(volatile uint16_t *word, const uint16_t value) {
  FCTL3 = FLASH_KEY | FCTL3_LOCKA; // unlock info memory
  FCTL1 = FLASH_KEY | FCTL1_WRITE;
  *word = value;
  FCTL1 = FLASH_KEY;
  FCTL3 = FLASH_KEY | FCTL3_LOCKA | FCTL3_LOCK;
}

#define RED_LED   (1U << 0U) // P1.0
#define GREEN_LED (1U << 6U) // P1.6
#define S2        (1U << 3U) // P1.3

// clang-format off
static uint16_t data[32] = {
#if 0
    // factory data from a MSP430G2211 device
    0xb2bcU, 0x26feU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0x10feU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0x0201U, 0x86baU
#else
    // aftermarket 16MHz calibration data for a MSP430G2231 device
    0xbdd6U, 0x26feU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0xffffU, 0xffffU, 0x0efeU, 0xffffU, 0xffffU,
    0xffffU, 0xffffU, 0xffffU, 0x0801U, 0x8f8aU, 0xffffU, 0xffffU, 0x86c8U
#endif
};
// clang-format on

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  // Choosing a farily low processor frequency here, because that way it's
  // easier to drive the flash timing generator.
  BCSCTL1 = 0x60U;
  DCOCTL = 0xc8U;
#define SMCLK_FREQUENCY (300000UL) // 0.3 MHz typically

  // flash timing generator operation frequency must be in 257..476 kHz
  FCTL2 = FLASH_KEY | FCTL2_SMCLK | FCTL2_DIVIDE_BY(1);

  P1DIR = RED_LED | GREEN_LED;
  P1REN = S2;

  if (tlv_checksum(info.segment_a) != info.segment_a[0]) {
    P1OUT = RED_LED;
    // wait for user to confirm the error condition
    while (P1IN & S2) {
    }
    P1OUT = 0U;
  }

  data[0] = tlv_checksum(data);

  erase_segment_a();

  if (FCTL3 & FCTL3_FAIL) {
    P1OUT = RED_LED;
    for (;;) {
    }
  }

  for (int i = 0; i < 32; ++i) {
    write_segment_a(&info.segment_a[i], data[i]);

    if (FCTL3 & FCTL3_FAIL) {
      P1OUT = RED_LED;
      for (;;) {
      }
    }
  }

  if (tlv_checksum(info.segment_a) != info.segment_a[0]) {
    P1OUT = RED_LED;
    for (;;) {
    }
  }

  P1OUT = GREEN_LED;

  for (;;) {
  }
}

__attribute__((used, section(".vectors"))) static const struct vtable vt = {
    .reset = on_reset};
