// Start-up code and utilities for MSP430G2452 MCUs.

#include <stddef.h>
#include <stdint.h>

extern int main(void);

extern const volatile uint8_t P1IN;
extern volatile uint8_t P1OUT;
extern volatile uint8_t P1DIR;
extern volatile uint8_t P1IFG;
extern volatile uint8_t P1IES;
extern volatile uint8_t P1IE;
extern volatile uint8_t P1REN;

extern const volatile uint8_t P2IN;
extern volatile uint8_t P2OUT;
extern volatile uint8_t P2DIR;
extern volatile uint8_t P2IFG;
extern volatile uint8_t P2IES;
extern volatile uint8_t P2IE;
extern volatile uint8_t P2SEL;
extern volatile uint8_t P2REN;

extern volatile uint8_t BCSCTL3;
extern volatile uint8_t DCOCTL;
extern volatile uint8_t BCSCTL1;

extern volatile uint16_t WDTCTL;
#define WDT_UNLOCK (0x5a00U)
#define WDT_HOLD (0x0080U)

extern const uint8_t CAL_DCO_16MHz;
extern const uint8_t CAL_BC1_16MHz;

__attribute__((naked)) _Noreturn void on_reset() {
  // init stack pointer
  extern const uint16_t _stack;
  __asm__ volatile("mov %0, SP" : : "ri"(&_stack));

  // .data
  extern uint16_t _sdata[]; // .data start
  extern uint16_t _edata;   // .data end
  const ptrdiff_t data_count = &_edata - &_sdata[0];
  for (ptrdiff_t i = 0; i < data_count; ++i) {
    extern const uint16_t _sidata[]; // .data init values
    _sdata[i] = _sidata[i];
  }

  // .bss
  extern uint16_t _sbss[]; // .bss start
  extern uint16_t _ebss;   // .bss end
  const ptrdiff_t bss_count = &_ebss - &_sbss[0];
  for (ptrdiff_t i = 0; i < bss_count; ++i) {
    _sbss[i] = 0U;
  }

  // .preinit_array
  extern void (*_preinit_array_start[])();
  extern void (*_preinit_array_end[])();
  const ptrdiff_t num_preinits =
      &_preinit_array_end[0] - &_preinit_array_start[0];
  for (ptrdiff_t i = 0; i < num_preinits; ++i) {
    _preinit_array_start[i]();
  }

  // .init_array
  extern void (*_init_array_start[])();
  extern void (*_init_array_end[])();
  const ptrdiff_t num_inits = &_init_array_end[0] - &_init_array_start[0];
  for (ptrdiff_t i = 0; i < num_inits; ++i) {
    _init_array_start[i]();
  }

  main();
}

#define go_to_sleep()                                                          \
  do {                                                                         \
    __asm__ volatile("nop { bis %0, SR { nop" : : "ri"(0x10));                 \
  } while (0)

#define stay_awake()                                                           \
  do {                                                                         \
    __bic_SR_register_on_exit(0x10);                                           \
  } while (0)

void enable_interrupts() { __asm__ volatile("eint"); }
void disable_interrupts() { __asm__ volatile("dint { nop"); }

typedef void (*vector)();
__attribute__((interrupt)) void default_isr() {}
