// Start-up code and utilities for MSP430G2xx MCUs.

#include <stddef.h>
#include <stdint.h>

extern const volatile uint8_t P1IN;
extern volatile uint8_t P1OUT;
extern volatile uint8_t P1DIR;
extern volatile uint8_t P1IFG;
extern volatile uint8_t P1IES;
extern volatile uint8_t P1IE;
extern volatile uint8_t P1SEL;
extern volatile uint8_t P1REN;

extern const volatile uint8_t P2IN;
extern volatile uint8_t P2OUT;
extern volatile uint8_t P2DIR;
extern volatile uint8_t P2IFG;
extern volatile uint8_t P2IES;
extern volatile uint8_t P2IE;
extern volatile uint8_t P2SEL;
extern volatile uint8_t P2REN;

extern volatile uint16_t USICTL;
#define USI_IE     (0x1000U) // interrupt enable
#define USI_IFG    (0x0100U) // interrupt flag
#define USI_PE6    (0x0040U)
#define USI_LSB    (0x0010U) // LSB first
#define USI_MASTER (0x0008U)
#define USI_OE     (0x0002U) // output enable
#define USI_RESET  (0x0001U)
extern volatile uint16_t USICCTL;
#define USI_SMCLK       (0x0008U)
#define USI_TACCR0      (0x0014U)
#define USI_COUNT(bits) (((bits) & 0xfU) << 8U)
extern volatile uint8_t USICNT;
#define USI_16BIT (0x40U)
extern volatile uint16_t USISR;
extern volatile uint8_t USISRL;

extern volatile uint8_t BCSCTL3;
extern volatile uint8_t DCOCTL;
extern volatile uint8_t BCSCTL1;

extern volatile uint16_t WDTCTL;
#define WDT_UNLOCK (0x5a00U)
#define WDT_HOLD   (0x0080U)
#define WDT_CLEAR  (0x0008U)
#define WDT_ACLK   (0x0040U)
#define WDT_32768  (0x0001U)
#define WDT_8192   (0x0001U)
#define WDT_512    (0x0002U)
#define WDT_64     (0x0003U)

#define FLASH_KEY  (0xa500U)
extern volatile uint16_t FCTL1;
#define FCTL1_WRITE (0xa540U)
#define FCTL1_ERASE (0xa502U)
extern volatile uint16_t FCTL2;
#define FCTL2_SMCLK        (0x0080U)
#define FCTL2_DIVIDE_BY(x) ((x)-1)
extern volatile uint16_t FCTL3;
#define FCTL3_FAIL  (0x0080U)
#define FCTL3_LOCKA (0x0040U)
#define FCTL3_LOCK  (0x0010U)
#define FCTL3_WAIT  (0x0008U)

extern volatile uint16_t TACTL;
#define TACTL_SMCLK (0x0200U)
#define TACTL_UP    (0x0010U) // start counting up to TACCR0
#define TACTL_START(mode)                                                      \
  do {                                                                         \
    TACTL |= (mode);                                                           \
  } while (0)
#define TACTL_STOP()                                                           \
  do {                                                                         \
    TACTL &= ~0x0030U;                                                         \
  } while (0)
#define TACTL_IE  (0x0002U) // enable the `timer*_a3` interrupt
#define TACTL_IFG (0x0001U) // flag for the `timer*_a3` interrupt
extern volatile uint16_t TACCTL0;
#define TACCTL0_OUTMODE_TOGGLE (0x0080U)
extern const volatile uint16_t TAR;
extern volatile uint16_t TACCR0;

extern const uint8_t CAL_DCO_16MHz;
extern const uint8_t CAL_BC1_16MHz;
extern const uint8_t CAL_DCO_1MHz;
extern const uint8_t CAL_BC1_1MHz;

extern int main(void);

__attribute__((naked)) _Noreturn void on_reset(void) {
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

#define enable_interrupts()                                                    \
  do {                                                                         \
    __asm__ volatile("nop { eint { nop");                                      \
  } while (0)

#define disable_interrupts()                                                   \
  do {                                                                         \
    __asm__ volatile("dint { nop");                                            \
  } while (0)

typedef void (*vector)(void);
