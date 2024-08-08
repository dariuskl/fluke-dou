# Data Output Units for Vintage Fluke Meters

Jumbo builds
targeting MSP430G2xx

- easy-to-use
- cheap
- obtainable
- most modern 5V parts just have internal regulators due to lithography,
  so we'd just be saving us the level shifting, but for some devices
  conditioning is required anyway
- simply C code that should be easily portable to other devices

## 8000A

**Firmware** — first working version available with tag `8000a-fw-1`
**Hardware** — PCB not yet designed, working on perfboard prototype

The 8000A is a very early multimeter with limited data output capabilities. It
offers only signals for the digits, none for the decimal point or range and
mode information.

The serial output is default configured to 19200 baud 7N1 and transmits the
readings as outlined below.

    <overload><polarity><MSD><2SD><3SD><LSD>\r\n

### Modifications Required for Battery Pack (Option -01)

The battery pack PCB does not have routing for all signals required by the DOU
onto the finger connector. The two-layered board just could not accommodate
both the traces required for the batteries and the DOU. This can be fixed with
a few jumper wires, though.

**TODO: insert a schema showing what points to connect here**

### Finger Connector Pinout

The signal `nT` is actually not part of the finger connector, but it does have
a strategically placed via. As it is even above pin 1 of the finger connector,
I assigned it the pin number zero.

Not all signals have suitable logic levels to interface to a 5 V µC (let alone
the 3.3 V MSP430), so comparators are used to condition the signals according
to the thresholds that I determined for each signal.

| Pin | Signal | Description        | Low level | High level  | Threshold |
|-----|--------|--------------------|-----------|-------------|-----------|
| 0   | nT     | Measurement period | 1.2 V     | 4.8 V       | 2.5 V     |
| 2   | nZERO  | ???                |           |             |           |
| 4   | S1     | MSD strobe         | 0 V       | 4.6 V       | 2.5 V     |
| 6   | S4     | LSD strobe         | 0 V       | 3.3 V       | 1.6 V     |
| 8   | S      | Strobe clock       | 0 V       | 2.5...4.6 V | 1.6 V     |
| 10  | +5V    | Regulated 5V rail  | —         | —           | —         |
| 12  | GND    | Ground             | —         | —           | —         |
| 14  | Y      | BCD 4              | 0 V       | 4.9 V       | 2.5 V     |
| 16  | X      | BCD 2              | 0 V       | 4.9 V       | 2.5 V     |
| 18  | W      | BCD 8              | 0 V       | 4.9 V       | 2.5 V     |
| 20  | Z      | BCD 1              | 0 V       | 4.9 V       | 2.5 V     |

### Protocol

| Digit     | W        | X     | Y        | Z     | Strobe |
|-----------|----------|-------|----------|-------|--------|
| DS1 (MSD) | overload | ?     | polarity | MSD   | S1     |
| DS2..DS3  | BCD 8    | BCD 4 | BCD 2    | BCD 1 | none   |
| DS4 (LSD) | BCD 8    | BCD 4 | BCD 2    | BCD 1 | S4     |
