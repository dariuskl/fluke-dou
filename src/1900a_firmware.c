// MSP430G2452-based firmware for the 1900A DOU.
// Drives the decode logic in `1900a.c`.

#include "1900a.c"
#include "msp430g2452.c"

#include <stdbool.h>

// port 1

#define OUT_B_MASK 1U // BCD 2
#define AS_1_MASK 2U  // LSD
#define TX_MASK 4U
#define RNG_2_MASK 8U
#define NML_MASK 0x10U
#define OVFL_MASK 0x20U
#define AS_3_MASK 0x40 // 4SD
#define AS_2_MASK 0x80 // 5SD

// port 2

#define NMUP_MASK 1U
#define OUT_C_MASK 2U    // BCD 4
#define OUT_D_MASK 4U    // BCD 8
#define AS_6_MASK 8U     // MSD
#define AS_5_MASK 0x10U  // 2SD
#define AS_4_MASK 0x20U  // 3SD
#define OUT_A_MASK 0x40U // BCD 1
#define DS_MASK 0x80U

static struct input_state decode_signals(const uint8_t port1,
                                         const uint8_t port2) {
  return {static_cast<i8>(
              ((port1 & as_1_mask_) != u8{0})
                  ? 1
                  : (((port1 & as_2_mask_) != u8{0})
                         ? 2
                         : (((port1 & as_3_mask_) != u8{0})
                                ? 3
                                : (((port2 & as_4_mask_) != u8{0})
                                       ? 4
                                       : (((port2 & as_5_mask_) != u8{0})
                                              ? 5
                                              : (((port2 & as_6_mask_) != u8{0})
                                                     ? 6
                                                     : 0)))))),
          static_cast<i8>((((port2 & out_a_mask_) != u8{0}) ? u8{1} : u8{0}) |
                          (((port1 & out_b_mask_) != u8{0}) ? u8{2} : u8{0}) |
                          (((port2 & out_c_mask_) != u8{0}) ? u8{4} : u8{0}) |
                          (((port2 & out_d_mask_) != u8{0}) ? u8{8} : u8{0})),
          (port2 & ds_mask_) != u8{0},
          (port1 & ovfl_mask_) != u8{0},
          (port1 & nml_mask_) != u8{0},
          (port1 & rng_2_mask_) != u8{0}};
}

void enable_nmup_interrupt() { P2IE |= NMUP_MASK; }
void disable_nmup_interrupt() { P2IE &= ~NMUP_MASK; }

int main(void) {
  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  // The watchdog timer will reset the device, if no nMUP signal has been
  // detected after expiration of the longest gate time of 10 seconds.
  WDTCTL = WDT_UNLOCK | WDT_HOLD;
  // TODO set up watchdog

  BCSCTL1 = CAL_BC1_16MHz;
  DCOCTL = CAL_DCO_16MHz;
  BCSCTL3 = 0x24U; // ACLK = VLOCLK

  P1OUT = 0U;
  P1DIR = TX_MASK;
  P1IES = 0U;
  P1REN = 0U;

  P2DIR = 0U;
  P2IES = NMUP_MASK;
  P2IE = NMUP_MASK;
  P2REN = 0U;

  enable_interrupts();
  go_to_sleep();

  for (;;) {
    const uint8_t port1 = P1IN;
    const uint8_t port2 = P2IN;

    const bool update_memory = (port2 & NMUP_MASK) == 0U;
    const struct input_state input_state = decode_signals(port1, port2);

    if (update_memory) {
      decoder.transit(input_state);
    } else {
      disable_nmup_interrupt();

      const auto buffer = std::string_view{decoder.reading()};

      uart_timer.start_up(u16{(smclk_frequency_Hz / serial_baud_rate) - 1});

      for (const auto &character : buffer) {
        serial.init_transmission(character);
        unsigned bit = 0U;
        while (serial.get_next_bit(bit)) {
          // Wait for the first and next bit.
          go_to_sleep();
          P1OUT = (P1OUT & ~TX_MASK) | (bit != 0U ? 0U : TX_MASK);
        }
        // Wait for the last bit.
        go_to_sleep();
        P1OUT = P1OUT & ~TX_MASK;
      }
      uart_timer.stop();
      decoder = {};

      // Wait for a falling edge on /MUP.
      enable_nmup_interrupt();
      go_to_sleep();
    }
  }
}

// Called on falling edge on /MUP.
__attribute__((interrupt)) void on_strobe() {
  P2IFG = 0U;
  stay_awake();
}

__attribute__((interrupt)) void on_timer() { stay_awake(); }

__attribute__((used, section(".vectors"))) static const vector vtable_[32] = {
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    on_strobe,   on_strobe, default_isr, default_isr, nullptr, nullptr,
    default_isr, on_timer,  default_isr, default_isr, nullptr, nullptr,
    default_isr, on_reset};
