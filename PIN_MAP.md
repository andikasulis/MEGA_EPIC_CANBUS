# Pin Mapping — MEGA_EPIC_CANBUS v2

## Ringkasan Cepat

| Kategori | Pin | Jumlah | Hash Variable |
|----------|-----|--------|---------------|
| CAN | D9 (CS), D50-D53 (SPI) | 5 | — |
| VSS Wheel Speed | D18-D21 | 4 | FrontLeft, FrontRight, RearLeft, RearRight |
| Gear Selector | D22-D26 | 5 | detectedGear |
| Digital Input | D27-D37 | 11 | MEGA_EPIC_1_D20_D34 (bits 5-15) |
| Analog Input | A0-A15 | 16 | MEGA_EPIC_1_A0 s/d A15 |
| AFR Databox | D14 (TX3), D15 (RX3) | 2 | CAN ID 0x190/0x191 |
| GPS | D16 (TX2), D17 (RX2) | 2 | 8 GPS variables |
| Slow GPIO Output | D39-D43, D47-D49 | 8 | MEGA_EPIC_1_OUT_SLOW (bits 0-7) |
| PWM Output | D3, D5-D8, D11, D12, D44-D46 | 10 | MEGA_EPIC_1_OUT_SLOW (bits 8-17) |
| Spare | D0, D1, D10, D38 | 4 | — |

---

## Digital Pins — Detail

| Pin | Fungsi | Timer/IRQ | Mode | Hash/CAN ID | Catatan |
|-----|--------|-----------|------|-------------|---------|
| D0 | RX0 | — | Spare | — | Tidak dipakai firmware |
| D1 | TX0 | — | Spare | — | Tidak dipakai firmware |
| D2 | MCP2515 INT | INT4 | Spare | — | Belum dipakai (polling mode) |
| D3 | PWM Output | Timer3 | OUTPUT | OUT_SLOW bit 8 | `MEGA_EPIC_1_PWM_T3_D3` |
| D4 | PWM-capable | Timer0 | — | — | ~1kHz, jangan reconfig (Arduino time) |
| D5 | PWM Output | Timer3 | OUTPUT | OUT_SLOW bit 9 | `MEGA_EPIC_1_PWM_T3_D5` |
| D6 | PWM Output | Timer4 | OUTPUT | OUT_SLOW bit 10 | `MEGA_EPIC_1_PWM_T4_D6` |
| D7 | PWM Output | Timer4 | OUTPUT | OUT_SLOW bit 11 | `MEGA_EPIC_1_PWM_T4_D7` |
| D8 | PWM Output | Timer4 | OUTPUT | OUT_SLOW bit 12 | `MEGA_EPIC_1_PWM_T4_D8` |
| D9 | MCP_CAN CS | Timer2 | SPI CS | — | **RESERVED** untuk CAN shield |
| D10 | Spare PWM | Timer2 | — | — | Available |
| D11 | PWM Output | Timer1 | OUTPUT | OUT_SLOW bit 13 | `MEGA_EPIC_1_PWM_T1_D11` |
| D12 | PWM Output | Timer1 | OUTPUT | OUT_SLOW bit 14 | `MEGA_EPIC_1_PWM_T1_D12` |
| D13 | LED | Timer0 | — | — | Onboard LED |
| D14 | **AFR TX3** | — | Serial3 TX | — | Ke AFR controller RX |
| D15 | **AFR RX3** | — | Serial3 RX | — | Ke AFR controller TX |
| D16 | GPS TX2 | — | Serial2 TX | — | Ke GPS RX |
| D17 | GPS RX2 | — | Serial2 RX | — | Ke GPS TX |
| D18 | **VSS FrontLeft** | INT3 | INPUT_PULLUP | -1645222329 | Falling edge, pullup |
| D19 | **VSS FrontRight** | INT2 | INPUT_PULLUP | 1549498074 | Falling edge, pullup |
| D20 | **VSS RearLeft** | INT1 | INPUT_PULLUP | 768443592 | I2C disabled |
| D21 | **VSS RearRight** | INT0 | INPUT_PULLUP | -403905157 | I2C disabled |
| D22 | **Gear Neutral** | — | INPUT_PULLUP | detectedGear=0 | Active LOW |
| D23 | **Gear 1** | — | INPUT_PULLUP | detectedGear=1 | Active LOW |
| D24 | **Gear 2** | — | INPUT_PULLUP | detectedGear=2 | Active LOW |
| D25 | **Gear 3** | — | INPUT_PULLUP | detectedGear=3 | Active LOW |
| D26 | **Gear 4** | — | INPUT_PULLUP | detectedGear=4 | Active LOW |
| D27 | **Digital In bit 5** | — | INPUT_PULLUP | D20_D34 bit 5 | Active LOW |
| D28 | **Digital In bit 6** | — | INPUT_PULLUP | D20_D34 bit 6 | Active LOW |
| D29 | **Digital In bit 7** | — | INPUT_PULLUP | D20_D34 bit 7 | Active LOW |
| D30 | **Digital In bit 8** | — | INPUT_PULLUP | D20_D34 bit 8 | Active LOW |
| D31 | **Digital In bit 9** | — | INPUT_PULLUP | D20_D34 bit 9 | Active LOW |
| D32 | **Digital In bit 10** | — | INPUT_PULLUP | D20_D34 bit 10 | Active LOW |
| D33 | **Digital In bit 11** | — | INPUT_PULLUP | D20_D34 bit 11 | Active LOW |
| D34 | **Digital In bit 12** | — | INPUT_PULLUP | D20_D34 bit 12 | Active LOW |
| D35 | **Digital In bit 13** | — | INPUT_PULLUP | D20_D34 bit 13 | Active LOW |
| D36 | **Digital In bit 14** | — | INPUT_PULLUP | D20_D34 bit 14 | Active LOW |
| **D37** | **Clutch Switch (bit 15)** | — | INPUT_PULLUP | D20_D34 bit 15 | Switch ke GND saat ditekan |
| D38 | Spare GPIO | — | — | — | Candidate future output |
| D39 | **Slow GPIO bit 0** | — | OUTPUT | OUT_SLOW bit 0 | `MEGA_EPIC_1_SLOW_D39` |
| D40 | **Slow GPIO bit 1** | — | OUTPUT | OUT_SLOW bit 1 | `MEGA_EPIC_1_SLOW_D40` |
| D41 | **Slow GPIO bit 2** | — | OUTPUT | OUT_SLOW bit 2 | `MEGA_EPIC_1_SLOW_D41` |
| D42 | **Slow GPIO bit 3** | — | OUTPUT | OUT_SLOW bit 3 | `MEGA_EPIC_1_SLOW_D42` |
| D43 | **Slow GPIO bit 4** | — | OUTPUT | OUT_SLOW bit 4 | `MEGA_EPIC_1_SLOW_D43` |
| D44 | **PWM Output** | Timer5 | OUTPUT | OUT_SLOW bit 15 | `MEGA_EPIC_1_PWM_T5_D44` |
| D45 | **PWM Output** | Timer5 | OUTPUT | OUT_SLOW bit 16 | `MEGA_EPIC_1_PWM_T5_D45` |
| D46 | **PWM Output** | Timer5 | OUTPUT | OUT_SLOW bit 17 | `MEGA_EPIC_1_PWM_T5_D46` |
| D47 | **Slow GPIO bit 5** | — | OUTPUT | OUT_SLOW bit 5 | `MEGA_EPIC_1_SLOW_D47` |
| D48 | **Slow GPIO bit 6** | — | OUTPUT | OUT_SLOW bit 6 | `MEGA_EPIC_1_SLOW_D48` |
| D49 | **Slow GPIO bit 7** | — | OUTPUT | OUT_SLOW bit 7 | `MEGA_EPIC_1_SLOW_D49` |
| D50 | MISO | SPI | SPI | — | **RESERVED** MCP_CAN |
| D51 | MOSI | SPI | SPI | — | **RESERVED** MCP_CAN |
| D52 | SCK | SPI | SPI | — | **RESERVED** MCP_CAN |
| D53 | SS | SPI | SPI | — | **RESERVED** MCP_CAN |

---

## Analog Pins — Detail

| Pin | Fungsi | Mode | Hash Variable | Catatan |
|-----|--------|------|---------------|---------|
| A0 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A0 (595545759) | 0-5V, 10-bit ADC |
| A1 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A1 (595545760) | 0-5V, 10-bit ADC |
| A2 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A2 (595545761) | 0-5V, 10-bit ADC |
| A3 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A3 (595545762) | 0-5V, 10-bit ADC |
| A4 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A4 (595545763) | 0-5V, 10-bit ADC |
| A5 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A5 (595545764) | 0-5V, 10-bit ADC |
| A6 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A6 (595545765) | 0-5V, 10-bit ADC |
| A7 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A7 (595545766) | 0-5V, 10-bit ADC |
| A8 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A8 (595545767) | 0-5V, 10-bit ADC |
| A9 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A9 (595545768) | 0-5V, 10-bit ADC |
| A10 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A10 (-1821826352) | 0-5V, 10-bit ADC |
| A11 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A11 (-1821826351) | 0-5V, 10-bit ADC |
| A12 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A12 (-1821826350) | 0-5V, 10-bit ADC |
| A13 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A13 (-1821826349) | 0-5V, 10-bit ADC |
| A14 | Analog Input | INPUT_PULLUP | MEGA_EPIC_1_A14 (-1821826348) | 0-5V, 10-bit ADC |
| **A15** | **Quickshift Load Cell** | **INPUT** | MEGA_EPIC_1_A15 (-1821826347) | **3.5-3.8V, JANGAN pullup** |

---

## CAN Bus — Frame Mapping

| CAN ID | Tipe | DLC | Data | Direction |
|--------|------|-----|------|-----------|
| 0x701 | Variable Request | 4 | Hash (int32) | Mega → ECU |
| 0x721 | Variable Response | 8 | Hash + Value (float32) | ECU → Mega |
| 0x781 | Variable Set | 8 | Hash + Value (float32) | Mega → ECU |
| 0x190 | Wideband Lambda | 8 | Version + Lambda + Temp | Mega → ECU |
| 0x191 | Wideband Diagnostic | 8 | ESR + Status + Duty | Mega → ECU |

### Variable Hash yang Dikirim via CAN

| Variable | Hash (int32) | Tipe | Interval |
|----------|--------------|------|----------|
| MEGA_EPIC_1_A0 | 595545759 | float (ADC) | 25ms/500ms |
| MEGA_EPIC_1_A1 | 595545760 | float (ADC) | 25ms/500ms |
| ... | ... | ... | ... |
| MEGA_EPIC_1_A15 | -1821826347 | float (ADC) | 25ms/500ms |
| MEGA_EPIC_1_D20_D34 | 2136453598 | uint16 (bitfield) | 25ms/500ms |
| detectedGear | 283558758 | uint8 | 25ms/500ms |
| MEGA_EPIC_1_OUT_SLOW | 1430780106 | uint32 (bitfield) | Request/Response |
| VSS FrontLeft | -1645222329 | float (PPS) | 25ms/500ms |
| VSS FrontRight | 1549498074 | float (PPS) | 25ms/500ms |
| VSS RearLeft | 768443592 | float (PPS) | 25ms/500ms |
| VSS RearRight | -403905157 | float (PPS) | 25ms/500ms |
| GPS HMSD Packed | 703958849 | uint32 | 25ms/500ms |
| GPS MYQSAT Packed | -1519914092 | uint32 | 25ms/500ms |
| GPS Accuracy | -1489698215 | float | 25ms/500ms |
| GPS Altitude | -2100224086 | float | 25ms/500ms |
| GPS Course | 1842893663 | float | 25ms/500ms |
| GPS Latitude | 1524934922 | float | 25ms/500ms |
| GPS Longitude | -809214087 | float | 25ms/500ms |
| GPS Speed | -1486968225 | float | 25ms/500ms |

---

## Wiring Diagram

```
                        Arduino Mega2560
                    ┌─────────────────────┐
                    │                     │
    CAN Shield ───►│ D9 (CS)             │
    SPI Header ───►│ D50-53 (SPI)        │
                    │                     │
    GPS TX ────────│ D17 (RX2)           │
    GPS RX ◄───────│ D16 (TX2)           │
                    │                     │
    AFR TX ────────│ D15 (RX3)           │
    AFR RX ◄───────│ D14 (TX3)           │
                    │                     │
    VSS FL ────────│ D18 (INT3)          │
    VSS FR ────────│ D19 (INT2)          │
    VSS RL ────────│ D20 (INT1)          │
    VSS RR ────────│ D21 (INT0)          │
                    │                     │
    Gear N ────────│ D22                 │
    Gear 1 ────────│ D23                 │
    Gear 2 ────────│ D24                 │
    Gear 3 ────────│ D25                 │
    Gear 4 ────────│ D26                 │
                    │                     │
    Button 1-11 ──►│ D27-D37             │
                    │                     │
    Quickshift ────│ A15 (INPUT only!)   │
    Analog 0-14 ──►│ A0-A14              │
                    │                     │
    Output 0-7 ───►│ D39-D43, D47-D49    │
    PWM 0-9 ──────►│ D3,D5-D8,D11,D12,  │
                    │ D44-D46             │
                    │                     │
                    │ GND ──────────────►│ GND
                    │ 5V  ──────────────►│ VCC
                    └─────────────────────┘
```

---

## Catatan Penting

1. **A15 = Quickshift** → Harus pakai `INPUT` (bukan `INPUT_PULLUP`) karena sensor memberikan voltage aktif 3.5-3.8V
2. **D20/D21** → I2C disabled, dipakai untuk VSS (interrupt)
3. **D37 = Clutch switch** → Switch ke GND saat ditekan, bit 15 dari `MEGA_EPIC_1_D20_D34`
4. **D9** → Reserved untuk CAN CS, jangan dipakai untuk GPIO lain
5. **D0/D1** → UART0, bisa dipakai sebagai GPIO tapi kehilangan Serial debug
6. **D14/D15** → Serial3 untuk AFR Databox, tidak bisa dipakai untuk GPIO lain
7. **D16/D17** → Serial2 untuk GPS, tidak bisa dipakai untuk GPIO lain
