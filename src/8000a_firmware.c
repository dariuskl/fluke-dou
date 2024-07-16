// MSP430G2452-based firmware for the 8000A DOU.

// (All durations given in ms.)
//                              ⬐ 0 to 12.8
//        ├──←  100 →──┼← 50 →┼←→┼────────────← 0.8 to 12.8 →────────────┤
//        ┍━━━━━━━━━━━━┑      ┍━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// nT ━━━━┙            ┕━━━━━━┙  ┊                                       ┊
//                               ┍━━━━━━┑                                ┊
// S1 ━━━━━━━━━━━━━━━━━━━━━━━━━━━┙      ┕━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//                                                             ┍━━━━━━┑  ┊
// S4 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┙      ┕━━━━━━━━
//    ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┑  ┍━━━━━━┑  ┍━━━━━━┑  ┍━━━━━━┑  ┍━━━━━
//  S                                   ┕━━┙      ┕━━┙      ┕━━┙      ┕━━┙

#include "8000a.c"
#include "msp430g2452.c"

// Masks for the I/O ports with P1 in the lower eight bits and P2 in the upper
// eight bits.
enum signal {
  nZERO = 0x0001U,
  Y = 0x0002U,  // BCD 2
  X = 0x0004U,  // BCD 4
  W = 0x0008U,  // BCD 8
  Z = 0x0010U,  // BCD 1
  Tx = 0x0040U, // P1.6, SDO
  nT = 0x0100U,
  S1 = 0x0200U,
  S4 = 0x0400U,
  S = 0x0800U,
};

static unsigned wxyz(const unsigned input) {
  return (input & W ? READING_W : 0U) | (input & X ? READING_X : 0U) |
         (input & Y ? READING_Y : 0U) | (input & Z ? READING_Z : 0U);
}

static unsigned wait_for(enum signal signal) {
  P2IE = signal >> 8U;
  go_to_sleep();
  P2IE = 0U;
  return P1IN | P2IN << 8U;
}

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  TACTL = TACTL_SMCLK; // ues SMCLK for best resolution of serial baudrate

  P1OUT = 0U;
  P1DIR = Tx;
  P1IES = 0U;
  P1IE = 0U;
  P1SEL = Tx; // USI on P1.6
  P1REN = 0U;

  P2DIR = 0U;
  P2IES = 0U; // rising edges
  P2IE = 0U;
  P2REN = 0U;

  enable_interrupts();

  for (;;) {
    // The watchdog timer will reset the device, if no measurement has been
    // detected for a while.
    // ACLK = VLOCLK = max. 20 kHz, according to datasheet
    // 6 measurements per seconds, typically, according to service manual
    // 20 kHz / 8192 = ~2,4 Hz
    // gives enough headroom to transmit the reading at 19200 baud
    WDTCTL = WDT_UNLOCK | WDT_CLEAR | WDT_ACLK | WDT_8192;

    unsigned input = wait_for(nT);

    if ((input & nT) == 0U) {
      continue;
    }

    input = wait_for(S1);

    unsigned reading = wxyz(input);

    do {
      input = wait_for(S);
      reading = reading << 4U | wxyz(input);
    } while ((input & S4) == 0U);

    char text[MAX_READING_SIZE];
    print_reading(text, reading);

    // start timer for serial data clock
    TACCR0 = SMCLK_FREQUENCY / SERIAL_BAUD_RATE - 1;
    TACTL_START(TACTL_UP);

    // configure serial output
    USICCTL = USI_TACCR0 | USI_COUNT(SERIAL_DATA_BITS);
    USICTL = USI_PE6 | USI_LSB | USI_MASTER | USI_OE;
    // spurious interrupt can be triggered, if USIIE is set in reset
    while (USICTL & USI_RESET) {
    }
    USICTL |= USI_IE;

    for (int i = 0; i < MAX_READING_SIZE; ++i) {
      USISRL = text[i];
      USICNT = SERIAL_DATA_BITS;
      go_to_sleep();
    }

    USICTL = USI_RESET;
    TACTL_STOP();
  }
}

__attribute__((interrupt)) void on_port2(void) {
  P2IFG = 0U;
  stay_awake();
}

__attribute__((interrupt)) void on_usi(void) {
  USICTL &= ~USI_IFG;
  stay_awake();
}

__attribute__((used, section(".vectors"))) static const struct vtable vt = {
    .reset = on_reset, .port2 = on_port2, .usi = on_usi};
