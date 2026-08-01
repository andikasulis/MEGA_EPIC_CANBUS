# Arduino Nano Pin Assignment

Project: `NANO_EPIC_CANBUS` — minimal EPIC CAN sender for quickshifter + clutch.

## Used
| Pin | Function | Notes |
|-----|----------|-------|
| D10 | MCP2515 CS | SPI chip select, reserved |
| D11 | MOSI | SPI |
| D12 | MISO | SPI |
| D13 | SCK  | SPI |
| A0  | QuickShifter ADC | 0-5V analog input → `MEGA_EPIC_1_A0` |
| D4  | Clutch switch | `INPUT_PULLUP`, LOW = active, mapped to bit 0 of `MEGA_EPIC_1_D22_D37` |

## Available for future expansion
| Pin | Type | Notes |
|-----|------|-------|
| D2  | digital / INT0 | interrupt capable |
| D3  | digital / PWM / INT1 | interrupt + PWM capable |
| D5  | digital / PWM | |
| D6  | digital / PWM | |
| D7  | digital | |
| D8  | digital | |
| D9  | digital / PWM | |
| A1..A7 | analog / digital | A6/A7 are analog-only input |

## Do not use
| Pin | Reason |
|-----|--------|
| D0  | Serial RX |
| D1  | Serial TX |
| D10 | MCP2515 CS |
| D11-D13 | SPI |

