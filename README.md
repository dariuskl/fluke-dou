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

### Modifications Required for Battery Pack (Option -01)

### Theory of Operation

#### Signal Conditioning

| Signal     | Low   | High       | Threshold |
|------------|-------|------------|-----------|
| nT         | 1.2 V | 4.8 V      | 2.5 V     |
| S1         | 0.0 V | 4.6 V      | 2.5 V     |
| S4         | 0.0 V | 3.3 V      | 1.6 V     |
| S          | 0.0 V | 2.5..3.8 V | 1.6 V     |
| Y, X, W, Z | 0.2 V | 4.9 V      | 2.5 V     |
