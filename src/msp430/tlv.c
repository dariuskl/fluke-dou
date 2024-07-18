// Tag-Length-Value for MSP430F2xx and MSP430G2xx devices.

#include <stdint.h>

static uint16_t tlv_checksum(const volatile uint16_t seg_a[static 32]) {
  uint16_t sum = 0U;
  // skip first word as it contains the stored checksum
  for (int i = 1; i < 32; ++i) {
    sum = sum ^ seg_a[i];
  }
  return ~sum + 1; // two's complement
}
