// MSP430G2452-based firmware for the 8000A DOU.

#include "8000a.c"
#include "msp430/g2231.c"

// Masks for the I/O ports.
enum port1 {  // pin  | function
  Z = 0x01U,  // P1.0 | BCD 1 ╮
  Y = 0x02U,  // P1.1 | BCD 2 ├ digit
  X = 0x04U,  // P1.2 | BCD 4 │
  W = 0x08U,  // P1.3 | BCD 8 ╯
  T = 0x10U,  // P1.4 | inverted nT with fixed logic levels
  S = 0x20U,  // P1.5 | strobe clock
  Tx = 0x40U, // P1.6 | SDO
};
enum port2 {  // pin  | function
  S1 = 0x40U, // P2.6 | MSD (DS1) strobe
  S4 = 0x80U, // P2.7 | LSD (DS4) strobe
};

static unsigned capture_input(void) {
  const byte port1 = P1IN;
  const byte port2 = P2IN;
  return (port1 & Z ? INPUT_Z : 0U) | (port1 & Y ? INPUT_Y : 0U) |
         (port1 & X ? INPUT_X : 0U) | (port1 & W ? INPUT_W : 0U) |
         (port1 & T ? INPUT_T : 0U) | (port1 & S ? INPUT_S : 0U) |
         (port2 & S1 ? INPUT_S1 : 0U) | (port2 & S4 ? INPUT_S4 : 0U);
}

static void send_serial(const char msg[static MAX_READING_SIZE]);

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  // The higher the frequency, the better we can process the signals from the
  // device. However, the low cost chips such as the G2231 don't have factory
  // calibration for 16 MHz. So if one of those is to be used, the constants
  // have to be determined by hand. The `msp430/info_util.c` can be used to
  // store the constants to the info memory.
  BCSCTL1 = CAL_BC1_16MHz;
  DCOCTL = CAL_DCO_16MHz;
#define SMCLK_FREQUENCY (16000000UL)
  BCSCTL3 = 0x24U; // ACLK = VLOCLK

  TACTL = TACTL_SMCLK;  // use SMCLK for best resolution of serial baudrate
  USICCTL = USI_TACCR0; // clock serial via TimerA

  // 1. Make sure to pull Tx high ASAP.
  // 2. All inputs shall have pull-ups, because the comparator outputs are OD.
  P1OUT = Z | Y | X | W | T | S | Tx;
  P1DIR = Tx;
  P1IES = PxIES_FALLING_EDGE(T) | PxIES_RISING_EDGE(S);
  P1IFG = 0U; // setting PxIES could trigger interrupt
  P1SEL = 0U; // USI will be configured to P1.6 later to keep the line high
  P1REN = Z | Y | X | W | T | S; // enable resistors on all inputs

  P2OUT = S1 | S4; // all inputs shall have pull-ups
  P2DIR = 0U;
  P2REN = S1 | S4; // enable resistors on all inputs

  enable_interrupts();

  for (;;) {
    P1IE = T | S;
    struct decoder_state state = {0U, 0};
    for (; state.next_digit <= NUMBER_OF_DIGITS;
         state = decode(state, capture_input())) {
      // The watchdog timer will reset the device, if no measurement has been
      // detected for a while.
      // ACLK = VLOCLK = max. 20 kHz, according to datasheet
      // 6 measurements per seconds, typically, according to service manual
      // 20 kHz / 8192 = ~2,4 Hz
      // gives enough headroom to transmit the reading at 19200 baud
      // and allows to survive flashing display where there might be no digits
      //  in a reading.
      WDTCTL = WDT_UNLOCK | WDT_CLEAR | WDT_ACLK | WDT_8192;
      go_to_sleep();
    }
    P1IE = 0U;

    char text[MAX_READING_SIZE];
    print_reading(text, state.reading);

    send_serial(text);
  }
}

static void send_serial(const char msg[static MAX_READING_SIZE]) {
  // start timer for serial data clock
  TACCR0 = (SMCLK_FREQUENCY / SERIAL_BAUD_RATE) / 2;
  TACCTL0 = TACCTL0_OUTMODE_TOGGLE;
  TACTL_START(TACTL_UP);

  // configure serial output
  P1SEL |= Tx; // USI on P1.6
  USICCTL = USI_TACCR0;
  USICTL = USI_PE6 | USI_LSB | USI_MASTER | USI_OE;
  // spurious interrupt can be triggered, if USIIE is set in reset
  while (USICTL & USI_RESET) {
  }
  USICTL |= USI_IE;

  for (int i = 0; i < MAX_READING_SIZE && msg[i] != '\0'; ++i) {
    USISR = STOP_BIT | ((unsigned)msg[i] << 1U); // make space for the start bit
    // extra start & stop bits as we're in SPI mode --v
    USICNT = USI_16BIT | (SERIAL_DATA_BITS + 2);
    go_to_sleep();
  }

  TACTL_STOP();
}

__attribute__((interrupt)) void on_port1(void) {
  P1IFG = 0U;
  stay_awake();
}

__attribute__((interrupt)) void on_usi(void) {
  USICTL &= ~USI_IFG;
  stay_awake();
}

__attribute__((used, section(".vectors"))) static const struct vtable vt = {
    .reset = on_reset, .port1 = on_port1, .usi = on_usi};
