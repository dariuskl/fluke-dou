// MSP430G2452-based firmware for the 1900A DOU.
// Drives the decode logic in `1900a.c`.

#include "1900a.c"
#include "msp430g2452.c"

// Masks for the I/O ports with P1 in the lower eight bits and P2 in the upper
// eight bits.
enum signal {
  OUT_B = 0x0001U, // BCD 2
  AS_1 = 0x0002U,  // LSD strobe
  Tx = 0x0004U,
  RNG_2 = 0x0008U,
  NML = 0x0010U,
  OVFL = 0x0020U,
  AS_3 = 0x0040U, // 4SD strobe
  AS_2 = 0x0080U, // 5SD strobe
  nMUP = 0x0100U,
  OUT_C = 0x0200U, // BCD 4
  OUT_D = 0x0400U, // BCD 8
  AS_6 = 0x0800U,  // MSD strobe
  AS_5 = 0x1000U,  // 2SD strobe
  AS_4 = 0x2000U,  // 3SD strobe
  OUT_A = 0x4000U, // BCD 1
  DS = 0x8000U
};

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

void enable_nmup_interrupt() {
  P2IE |= NMUP_MASK;
}
void disable_nmup_interrupt() {
  P2IE &= ~NMUP_MASK;
}

static unsigned wait_for(enum signal signal) {
  P2IE = signal >> 8U;
  go_to_sleep();
  P2IE = 0U;
  return P1IN | P2IN << 8U;
}

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  BCSCTL1 = 0x8f;  // TODO CAL_BC1_16MHz;
  DCOCTL = 0x8a;   // TODO CAL_DCO_16MHz;
  BCSCTL3 = 0x24U; // ACLK = VLOCLK

#define SMCLK_FREQUENCY (16000000UL)

  TACTL = TACTL_SMCLK; // ues SMCLK for best resolution of serial baudrate

  P1OUT = 0U;
  P1DIR = Tx;
  P1IES = 0U;
  P1REN = 0U;

  P2DIR = 0U;
  P2IES = nMUP; // nMUP falling edge
  P2IE = 0U;
  P2REN = 0U;

  enable_interrupts();

  for (;;) {
    // The watchdog timer will reset the device, if no nMUP signal has been
    // detected after expiration of the longest gate time of 10 seconds.
    // TODO set up watchdog

    const unsigned input = wait_for(nMUP);

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

__attribute__((interrupt)) void on_timer() {
  stay_awake();
}

__attribute__((used, section(".vectors"))) static const vector vtable_[32] = {
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
    on_strobe,   on_strobe, default_isr, default_isr, nullptr, nullptr,
    default_isr, on_timer,  default_isr, default_isr, nullptr, nullptr,
    default_isr, on_reset};
