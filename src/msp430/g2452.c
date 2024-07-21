// Start-up code and utilities for MSP430G2452 MCUs.

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
  vector timer0_a3_2;
  vector timer0_a3;
  vector watchdog;
  vector comparator;
  vector v28_;
  vector v29_;
  vector nmi;
  vector reset;
};
_Static_assert(sizeof(struct vtable) == 32 * sizeof(void *),
               "vector table has unexpected size");
