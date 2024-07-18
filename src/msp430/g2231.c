// Start-up code and utilities for MSP430G2231 MCUs.

#include "g2xx.c"

struct vtable {
  vector unused_[16];
  vector v16_;
  vector v17_;
  vector port1;
  vector port2;
  vector usi;
  vector adc10;
  vector v22_;
  vector v23_;
  vector timer_a2_2;
  vector timer_a2;
  vector watchdog;
  vector v27_;
  vector v28_;
  vector v29_;
  vector nmi;
  vector reset;
};
_Static_assert(sizeof(struct vtable) == 32 * sizeof(void *),
               "vector table has unexpected size");
