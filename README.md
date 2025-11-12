# Second Display LCD Sender

ESP32-based wireless data transmitter for weight scales. This project reads weight data from weight scales via RS-232 serial communication and transmits it wirelessly to a receiver ESP32 using the ESP-NOW protocol.

## Features

- Reads serial data from weight scales (multiple output formats supported)
- Parses and formats weight data for LCD display (16 characters)
- Wireless transmission via ESP-NOW protocol
- Optional unit conversion from grams to tons (divide by 37.7994)
- Supports multiple weight balances (MODEMT1, MODEMT2, MODEAND)
- Button-activated unit conversion
- Stability indicator parsing (stable/dynamic readings)

## Hardware Requirements

- ESP32 DOIT DevKit v1
- weight scale with RS-232 serial output
- Push button (optional, for unit conversion)

## Pin Configuration

| Pin | Function | Description |
|-----|----------|-------------|
| GPIO 16 | RX | Serial receive from weight scale |
| GPIO 17 | TX | Serial transmit to weight scale |
| GPIO 15 | BUTTON_PIN | Unit conversion button (active HIGH) |
| GPIO 13 | MODEMT1_PIN | Mode T1 selection |
| GPIO 12 | MODEMT2_PIN | Mode T2 selection |
| GPIO 14 | MODEAND_PIN | Mode AND selection |

## Supported Data Formats

### MODEMT1 Format
- Length: 16 characters
- Pattern: `S S xxxxxxxx g` (stable) or `S D xxxxxxxx g` (dynamic)
- Example: `S S   1234.56 g`

### MODEMT2 Format
- Length: 14 characters
- Pattern: `S xxxxxxxx g` (stable) or `SD xxxxxxxx g` (dynamic)
- Example: `S   1234.56 g`

### MODEAND Format
- Length: 15 characters
- Pattern: `ST,+xxxxx  g` (stable) or `US,+xxxxx  g` (unstable)
- Supports negative values: `ST,-xxxxx  g`
- Example: `ST,+01234  g`

## Configuration

### Receiver MAC Address

Update the receiver ESP32's MAC address in [main.cpp:24](src/main.cpp#L24):

```cpp
uint8_t address[6] = {0xcc, 0xdb, 0xa7, 0x3e, 0xb7, 0xf8};
```

### Serial Communication

- Baud rate: 9600
- Data format: 8N1 (8 data bits, no parity, 1 stop bit)
- End marker: newline (`\n`)

## Usage

1. Connect the weight scale to the ESP32 via serial connection (RX/TX pins)
2. Power on the ESP32
3. The device will automatically read serial data from the scale
4. Data is parsed and transmitted via ESP-NOW to the configured receiver
5. Press the button (GPIO 15) to enable unit conversion from grams to tons (when applicable)

## Unit Conversion

When the button is pressed (and MODEAND_PIN is HIGH for MODEMT1 format), the weight is converted from grams to tons using the formula:

```
weight_in_tons = weight_in_grams / 37.7994
```

The output format changes from `g` to `t` suffix.

## Serial Monitor

Monitor output at 9600 baud to see:
- Received data from the scale
- Parsed and formatted weight values
- ESP-NOW delivery status (Success/Fail)

## Dependencies

- Arduino framework
- ESP32 Arduino Core
- ESP-NOW library (included in ESP32 core)

## Notes

- The project filters out initialization strings (e.g., `C308720092`)
- Stability indicators are preserved in the output (`*` for stable readings)
- Invalid or unrecognized data formats are silently ignored
