# DOKUMENTASI LENGKAP: MEGA_EPIC_CANBUS

**Versi:** 2.0 — Phase 1 + Merge GearIndicatorCan  
**Platform:** Arduino Mega2560 + MCP2515 CAN Shield  
**Protokol:** EPIC_CAN_BUS @ 500 kbps + rusEFI Wideband CAN (0x190/0x191)

---

## DAFTAR ISI

1. [PENDAHULUAN](#1-pendahuluan)
   - 1.1. Apa Itu MEGA_EPIC_CANBUS?
   - 1.2. Problem Statement
   - 1.3. Target Pengguna
2. [ARSITEKTUR SISTEM](#2-arsitektur-sistem)
   - 2.1. Three-Layer Architecture
   - 2.2. Data Flow Diagram
   - 2.3. Alur Kerja Utama
3. [HARDWARE](#3-hardware)
   - 3.1. BOM (Bill of Materials)
   - 3.2. Pin Mapping Lengkap
   - 3.3. Wiring Diagram (Deskripsi)
   - 3.4. Spesifikasi Hardware
4. [PROTOKOL EPIC_CAN_BUS](#4-protokol-epic_can_bus)
   - 4.1. CAN ID Mapping
   - 4.2. Variable Request/Response
   - 4.3. Variable Set (Fire-and-Forget)
   - 4.4. Function Call
   - 4.5. Data Types & Byte Order
   - 4.6. CAN Hardware Filter
5. [FIRMWARE — STRUKTUR KODE](#5-firmware--struktur-kode)
   - 5.1. File Mapping
   - 5.2. Konfigurasi & Constants
   - 5.3. Helper Functions (Endian Conversion)
   - 5.4. CAN Frame Helpers
6. [MODUL INPUT](#6-modul-input)
   - 6.1. Analog Input (A0–A15)
   - 6.2. Digital Input (D22–D37)
   - 6.3. VSS Wheel Speed (D18–D21)
   - 6.4. GPS Module (Serial2)
7. [MODUL OUTPUT](#7-modul-output)
   - 7.1. Slow GPIO (D39–D43, D47–D49)
   - 7.2. PWM Output (D3, D5–D8, D11, D12, D44–D46)
8. [SMART TRANSMISSION SYSTEM](#8-smart-transmission-system)
   - 8.1. Konsep
   - 8.2. State Machine
   - 8.3. Data Structures
   - 8.4. Thresholds
   - 8.5. CAN Bus Load Analysis
9. [NMEA PARSER (GPS)](#9-nmea-parser-gps)
   - 9.1. Supported Sentences
   - 9.2. Alur Parsing
   - 9.3. Coordinate Conversion
   - 9.4. GPS → CAN Packing
10. [TIMING & PERFORMANCE](#10-timing--performance)
    - 10.1. Timing Diagram
    - 10.2. CAN Bus Load Calculation
    - 10.3. Memory Usage
11. [VARIABLE HASH MAPPING](#11-variable-hash-mapping)
    - 11.1. Analog Inputs
    - 11.2. Digital Input
    - 11.3. VSS Channels
    - 11.4. GPS Variables
    - 11.5. Output Variables
12. [IMPLEMENTASI DETAIL](#12-implementasi-detail)
    - 12.1. Setup Sequence
    - 12.2. Main Loop Breakdown
    - 12.3. CAN RX Handler
    - 12.4. VSS Rate Calculation
13. [MODUL GEAR DETECTION](#13-modul-gear-detection)
    - 13.1. Pin Mapping
    - 13.2. Logika Deteksi
    - 13.3. Validasi
    - 13.4. CAN Transmission
14. [AFR DATABOX VIA SERIAL3](#14-afr-databox-via-serial3)
    - 14.1. Protocol Databox BRT
    - 14.2. Frame Format
    - 14.3. State Machine Reconnect
    - 14.4. Probe Temperature Calculation
    - 14.5. Pin Allocation
15. [rusEFI WIDEBAND CAN PROTOCOL](#15-rusefi-wideband-can-protocol)
    - 15.1. Frame 0x190 — Lambda & Temperature
    - 15.2. Frame 0x191 — Diagnostic
    - 15.3. Byte Order (Little Endian)
    - 15.4. Valid Flag & Error Handling
16. [CAN ERROR HANDLING](#16-can-error-handling)
    - 16.1. Clock Fallback
    - 16.2. Auto Re-init
    - 16.3. Max Retry & Recovery
    - 16.4. Rate-Limited Logging
17. [POTENSIAL MASALAH & BUG](#17-potensial-masalah--bug)
18. [ROADMAP & STATUS](#18-roadmap--status)
19. [REFERENSI](#19-referensi)

---

## 1. PENDAHULUAN

### 1.1. Apa Itu MEGA_EPIC_CANBUS?

MEGA_EPIC_CANBUS adalah **firmware Arduino Mega2560** yang berfungsi sebagai **modul ekspansi I/O berbasis CAN bus** untuk **epicEFI ECU** (Engine Control Unit open-source). Dengan menggunakan Arduino Mega2560 yang murah ($10-40) dan shield MCP2515 CAN ($5-20), proyek ini memungkinkan penambahan puluhan input/output sensor dan aktuator ke ECU tanpa perlu membeli modul ekspansi komersial yang mahal.

**Kemampuan I/O:**
- 16 analog input (A0–A15, 0–5V)
- 5 gear selector input (D22–D26, N/1/2/3/4)
- 11 digital input pushbutton (D27–D37, INPUT_PULLUP, inverted logic)
- 4 VSS wheel speed sensor (D18–D21, interrupt-driven)
- 1 GPS receiver (Serial2, NMEA-0183)
- 1 AFR wideband Databox (Serial3/D14-D15, 57600 baud)
- 8 slow GPIO output (D39–D43, D47–D49)
- 10 PWM output (D3, D5–D8, D11, D12, D44–D46)

### 1.2. Problem Statement

**Masalah:** ECU modern seperti epicEFI memiliki jumlah I/O fisik yang terbatas karena:
- Keterbatasan jumlah pin mikrokontroler
- Biaya penambahan pin dan sirkuit pendukung
- Keterbatasan ruang board
- Jumlah I/O fixed saat desain

**Solusi yang ada mahal:** Modul ekspansi I/O komersial:
- Protokol proprietary (terkunci vendor)
- Biaya $500-2000+
- Ketersediaan terbatas
- Sulit diperbaiki di lapangan

**Solusi MEGA_EPIC_CANBUS:**
- Hardware umum, murah, mudah didapat
- Protokol CAN bus standar (terbukti di industri otomotif)
- Open source, bisa dimodifikasi siapa saja
- Mudah diperbaiki dan diganti

### 1.3. Target Pengguna

| Segmen | Kebutuhan |
|--------|-----------|
| **DIY Engine Builder** | Ekspansi I/O murah untuk engine management custom |
| **Professional Tuner** | Solusi cost-effective untuk customer builds |
| **Educational/Research** | Platform embedded systems dengan protokol otomotif nyata |

---

## 2. ARSITEKTUR SISTEM

### 2.1. Three-Layer Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    I/O LAYER (Pin Management)                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐   │
│  │ Analog   │ │ Digital  │ │ VSS      │ │ GPS (Serial2) │   │
│  │ A0-A15   │ │ D22-D37  │ │ D18-D21  │ │ NMEA-0183     │   │
│  │ analogRd │ │ digRead  │ │ IRQ cnt  │ │ nmeaParser    │   │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └───────┬───────┘   │
│       │            │            │                │           │
│  ┌────▼────────────▼────────────▼────────────────▼───────┐   │
│  │            SMART TRANSMISSION LAYER                    │   │
│  │  ┌──────────┐  ┌───────────┐  ┌──────────────────┐   │   │
│  │  │ Change   │  │ State     │  │ Scheduler        │   │   │
│  │  │ Detection│─►│ Machine   │─►│ (25ms/500ms)     │   │   │
│  │  └──────────┘  └───────────┘  └────────┬─────────┘   │   │
│  └─────────────────────────────────────────┬─────────────┘   │
│                                            │                 │
│  ┌─────────────────────────────────────────▼─────────────┐   │
│  │              PROTOCOL LAYER (EPIC_CAN_BUS)             │   │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │   │
│  │  │ Variable     │ │ Variable     │ │ Function     │   │   │
│  │  │ Request/Resp │ │ Set (TX)     │ │ Call/Resp    │   │   │
│  │  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘   │   │
│  └─────────┼────────────────┼────────────────┼────────────┘   │
│            │                │                │                 │
│  ┌─────────▼────────────────▼────────────────▼─────────────┐   │
│  │              TRANSPORT LAYER (MCP_CAN / SPI)             │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │   │
│  │  │ MCP2515      │  │ SPI @ 8MHz  │  │ CAN 500kbps  │   │   │
│  │  │ CAN Controller│  │ (D50-D53)   │  │ (CAN_H/CAN_L)│   │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘   │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

**Penjelasan Layer:**
1. **I/O Layer** — Membaca/menulis pin fisik. Eksekusi langsung hardware.
2. **Smart Transmission** — Mendeteksi perubahan nilai, menentukan kapan transmit via state machine.
3. **Protocol Layer** — Membuat/memparse frame EPIC_CAN_BUS (variable_set, request, response).
4. **Transport Layer** — Komunikasi SPI dengan MCP2515, frame CAN di bus.

### 2.2. Data Flow Diagram

```
SENSORS                          CAN BUS                          ECU
───────                          ───────                          ───
A0-A15 ──┐
         ├── analogRead() ──┐
D22-D37 ─┤                  │
         ├── digitalRead() ─┤
D18-D21 ─┤                  │
         ├── ISR count ─────┤
Serial2 ─┤                  │
         ├── nmeaParser ────┤
                            ▼
                    ┌────────────────┐
                    │  Smart TX      │
                    │  State Machine │
                    │  29 channel    │
                    └───────┬────────┘
                            │ variable_set (0x781)
                            ▼
                    ┌────────────────┐
                    │  CAN Bus       │ ───── variable_set ─────► ECU menerima data
                    │  500 kbps      │ ◄──── variable_response ── ECU mengirim output
                    └────────────────┘
                            │ variable_request (0x701)
                            ▼
                    ┌────────────────┐
                    │  CAN RX        │
                    │  Handler       │
                    └───────┬────────┘
                            │ bitfield unpack
                            ▼
ACTUATORS
D39-43   ──► digitalWrite()
D47-49   ──► digitalWrite()
D3,5-8   ──► analogWrite()
D11,12   ──► analogWrite()
D44-46   ──► analogWrite()
```

### 2.3. Alur Kerja Utama

```
POWER ON
   │
   ▼
┌──────────────┐
│   setup()    │
│  ──────────  │
│  • CAN init  │
│  • Pin I/O   │
│  • Serial2   │
│  • ISR VSS   │
│  • State ini │
└──────┬───────┘
       │
       ▼
┌────────────────────────────────────────────┐
│               loop()                        │
│  ────────────────────────────────────────  │
│                                            │
│  ┌──────────────┐  ┌──────────────────┐   │
│  │ CAN RX Poll  │  │ VSS Calc Rate    │   │
│  │ readMessage  │  │ every 25ms       │   │
│  └──────┬───────┘  └────────┬─────────┘   │
│         │                   │              │
│  ┌──────▼───────────────────▼──────────┐   │
│  │  Output Request (25ms)              │   │
│  │  Request VAR_HASH_OUT_SLOW          │   │
│  └──────┬──────────────────────────────┘   │
│         │                                  │
│  ┌──────▼──────────────────────────────┐   │
│  │  GPS Read (every loop)              │   │
│  │  Serial2 → nmeaParserProcessChar()  │   │
│  └──────┬──────────────────────────────┘   │
│         │                                  │
│  ┌──────▼──────────────────────────────┐   │
│  │  Input Read (setiap 10ms)           │   │
│  │  • analogRead A0-A15                │   │
│  │  • digitalRead D22-D37              │   │
│  │  • VSS values (dari calc)           │   │
│  │  • Update state machine             │   │
│  └──────┬──────────────────────────────┘   │
│         │                                  │
│  ┌──────▼──────────────────────────────┐   │
│  │  Smart TX (every loop)              │   │
│  │  • 16 analog → if changed/stable    │   │
│  │  • 1 digital → if changed/stable    │   │
│  │  • 4 VSS     → if changed/stable    │   │
│  │  • 8 GPS     → if changed/stable    │   │
│  └─────────────────────────────────────┘   │
└────────────────────────────────────────────┘
```

---

## 3. HARDWARE

### 3.1. BOM (Bill of Materials)

| Komponen | Fungsi | Harga Estimasi |
|----------|--------|----------------|
| Arduino Mega2560 | Mikrokontroler utama | $10-40 |
| MCP2515 CAN Shield | Controller CAN via SPI | $5-20 |
| GPS Module GT-U7 / u-blox7 | Modul GPS via Serial2 | $8-15 |
| Kabel twisted pair | Bus CAN (CAN_H/CAN_L) | $2-5 |
| Resistor 120Ω × 2 | Terminasi CAN bus | $0.10 |
| Power supply 5V/2A | Catu daya | $5-10 |
| Kabel jumper | Wiring | $2-5 |
| **Total** | | **$32-95** |

### 3.2. Pin Mapping Lengkap

#### Digital Pins

| Pin | Label | Fungsi | Timer/IRQ | Catatan |
|-----|-------|--------|-----------|---------|
| D0 | RX0 | UART0 RX | - | Spare |
| D1 | TX0 | UART0 TX | - | Spare |
| D2 | INT4 | MCP2515 INT (CAN IRQ) | INT4 | Belum digunakan (polling) |
| D3 | PWM | `MEGA_EPIC_1_PWM_T3_D3` | Timer3 | PWM output |
| D4 | PWM | Spare (Timer0) | Timer0 | ~1kHz, jangan reconfig |
| D5 | PWM | `MEGA_EPIC_1_PWM_T3_D5` | Timer3 | PWM output |
| D6 | PWM | `MEGA_EPIC_1_PWM_T4_D6` | Timer4 | PWM output |
| D7 | PWM | `MEGA_EPIC_1_PWM_T4_D7` | Timer4 | PWM output |
| D8 | PWM | `MEGA_EPIC_1_PWM_T4_D8` | Timer4 | PWM output |
| D9 | CS | MCP_CAN Chip Select | Timer2 | RESERVED |
| D10 | PWM | Spare PWM | Timer2 | Available |
| D11 | PWM | `MEGA_EPIC_1_PWM_T1_D11` | Timer1 | PWM output |
| D12 | PWM | `MEGA_EPIC_1_PWM_T1_D12` | Timer1 | PWM output |
| D13 | LED | Onboard LED | Timer0 | ~1kHz |
| D14 | TX3 | UART3 TX | - | Spare |
| D15 | RX3 | UART3 RX | - | Spare |
| D16 | TX2 | UART2 TX | - | Spare |
| D17 | RX2 | UART2 RX | - | Spare |
| D18 | INT3 | **VSS FrontLeft** | INT3 | Falling edge, pullup |
| D19 | INT2 | **VSS FrontRight** | INT2 | Falling edge, pullup |
| D20 | INT1/SDA | **VSS RearLeft** | INT1 | I2C disabled, pullup |
| D21 | INT0/SCL | **VSS RearRight** | INT0 | I2C disabled, pullup |
| D22-D37 | GPIO | **Digital Input** (16-bit) | - | INPUT_PULLUP, inverted |
| D38 | GPIO | Spare | - | Candidate output |
| D39 | GPIO | **Slow GPIO output** | - | Bit 0 |
| D40 | GPIO | **Slow GPIO output** | - | Bit 1 |
| D41 | GPIO | **Slow GPIO output** | - | Bit 2 |
| D42 | GPIO | **Slow GPIO output** | - | Bit 3 |
| D43 | GPIO | **Slow GPIO output** | - | Bit 4 |
| D44 | PWM | **PWM output** | Timer5 | Bit 15 |
| D45 | PWM | **PWM output** | Timer5 | Bit 16 |
| D46 | PWM | **PWM output** | Timer5 | Bit 17 |
| D47 | GPIO | **Slow GPIO output** | - | Bit 5 |
| D48 | GPIO | **Slow GPIO output** | - | Bit 6 |
| D49 | GPIO | **Slow GPIO output** | - | Bit 7 |
| D50 | MISO | SPI MISO | SPI | RESERVED |
| D51 | MOSI | SPI MOSI | SPI | RESERVED |
| D52 | SCK | SPI SCK | SPI | RESERVED |
| D53 | SS | SPI SS | SPI | RESERVED |

#### Analog Pins

| Pin | Fungsi | Catatan |
|-----|--------|---------|
| A0-A15 | **Analog Input** (16 channel) | 0-5V, 10-bit ADC (0-1023) |

### 3.3. Wiring Diagram (Deskripsi)

**MCP2515 → Arduino Mega2560:**

```
MCP2515 Shield      Arduino Mega2560
─────────────────   ────────────────
INT (Interrupt)  ──► D2 (INT4)     [opsional untuk IRQ-driven]
CS (Chip Select) ──► D9            [WAJIB — hardcoded]
SI (MOSI)        ──► D51 (ICSP)   [WAJIB — SPI bus]
SO (MISO)        ──► D50 (ICSP)   [WAJIB — SPI bus]
SCK              ──► D52 (ICSP)   [WAJIB — SPI bus]
VCC              ──► 5V
GND              ──► GND
CAN_H            ──► CAN bus H (twisted pair)
CAN_L            ──► CAN bus L (twisted pair)
```

**CAN Bus:**
```
┌──────────┐                   ┌──────────┐
│ 120Ω     │                   │ 120Ω     │
│Terminator│                   │Terminator│
└────┬─────┘                   └────┬─────┘
     │                              │
     ├──── CAN_H (twisted pair) ────┤
     ├──── CAN_L (twisted pair) ────┤
     │                              │
┌────▼─────┐                  ┌────▼─────┐
│ MEGA_EPIC│                  │epicEFI   │
│ (Mega)   │                  │ECU       │
└──────────┘                  └──────────┘
```

**GPS Module (GT-U7) → Arduino Mega2560:**
```
GPS Module          Arduino Mega2560
──────────          ────────────────
TX ────────────────► RX2 (D17)
RX ◄──────────────── TX2 (D16)
VCC ────────────────► 5V
GND ────────────────► GND
```

**VSS VR Conditioner → Arduino Mega2560:**
```
VR Conditioner      Arduino Mega2560
──────────────      ────────────────
FL Signal ──────────► D18 (INT3) — FrontLeft
FR Signal ──────────► D19 (INT2) — FrontRight
RL Signal ──────────► D20 (INT1) — RearLeft
RR Signal ──────────► D21 (INT0) — RearRight
GND ────────────────► GND
```

### 3.4. Spesifikasi Hardware

| Komponen | Spesifikasi |
|----------|-------------|
| **MCU** | ATmega2560 @ 16 MHz, 8-bit AVR |
| **Flash** | 256 KB (~8 KB bootloader, ~248 KB available) |
| **SRAM** | 8 KB total (~1 KB stack, ~1 KB global, ~6 KB runtime) |
| **EEPROM** | 4 KB (belum digunakan) |
| **ADC** | 10-bit, ~100 µs per sample |
| **SPI** | 1 hardware SPI @ max 8 MHz |
| **CAN Controller** | MCP2515 via SPI |
| **CAN Transceiver** | TJA1050 / MCP2551 / SN65HVD230 |
| **CAN Speed** | 500 kbps |
| **CAN Frame** | Standard 2.0A (11-bit ID), 0-8 byte data |
| **CAN Buffer RX** | MCP2515: 2 buffer (harus dibaca cepat) |
| **CAN Buffer TX** | MCP2515: 3 buffer |
| **GPS** | GT-U7 / u-blox 7 / NEO-6M, NMEA-0183 |
| **GPS Baud** | 115200 default (9600 alternatif) |
| **GPS Rate** | 20 Hz (GT-U7), 5 Hz (NEO-6M max) |

---

## 4. PROTOKOL EPIC_CAN_BUS

### 4.1. CAN ID Mapping

Semua CAN ID menggunakan **11-bit standard ID** dengan per-ECU addressing:

| Operasi | Base ID | ID Aktual (ECU=1) | DLC | Deskripsi |
|---------|---------|-------------------|-----|-----------|
| Variable Request | `0x700` | `0x701` | 4 | Mega minta nilai variabel dari ECU |
| Variable Response | `0x720` | `0x721` | 8 | ECU mengirim nilai variabel |
| Function Request | `0x740` | `0x741` | 6-8 | Mega panggil fungsi ECU |
| Function Response | `0x760` | `0x761` | 8 | ECU mengirim hasil fungsi |
| Variable Set | `0x780` | `0x781` | 8 | Mega kirim data ke ECU (fire-and-forget) |

### 4.2. Variable Request/Response

**Pattern:** Mega meminta nilai variabel → ECU merespon dengan nilainya.

**Request Frame (Mega → ECU):**
```
CAN ID: 0x701
DLC: 4
Data: [Byte0] [Byte1] [Byte2] [Byte3]
       └───────── VarHash (int32, big-endian) ─────────┘
```

**Response Frame (ECU → Mega):**
```
CAN ID: 0x721
DLC: 8
Data: [Byte0..3]           [Byte4..7]
       └── VarHash (int32) ──┘ └── Value (float32) ──┘
       (echo dari request)       (big-endian)
```

**Contoh — Request variabel output slow GPIO:**
```
TX: 0x701  [55 44 33 0A]    // VAR_HASH_OUT_SLOW = 1430780106 = 0x5544330A
RX: 0x721  [55 44 33 0A] [42 48 00 00]  // 50.0f = 0x42480000
```

### 4.3. Variable Set (Fire-and-Forget)

**Pattern:** Mega mengirim nilai ke ECU tanpa ACK. ECU harus sudah enable `epicCanAllowSetVar`.

**Set Frame (Mega → ECU):**
```
CAN ID: 0x781
DLC: 8
Data: [Byte0..3]           [Byte4..7]
       └── VarHash (int32) ──┘ └── Value (float32) ──┘
                               (uint32 untuk packed value)
```

**Contoh — Kirim nilai analog A0:**
```
0x781  [23 7B B9 1F] [44 42 40 00]  // hash A0=595545759, value=777.0f
```

### 4.4. Function Call

**Pattern:** Mega memanggil fungsi di ECU dan menerima return value.

**Request Frame:**
```
CAN ID: 0x741
DLC: 6 atau 8
Data: [Byte0..1]           [Byte2..5]           [Byte6..7]
       └── FuncID (uint16) ─┘ └── Arg1 (float32) ─┘ └ Arg2 (int16) ┘
                               (opsional, jika DLC=8)
```

**Response Frame:**
```
CAN ID: 0x761
DLC: 8
Data: [Byte0..1]  [Byte2..3]  [Byte4..7]
       └ FuncID ─┘ └ Reserved ┘ └── Return Value (float32) ──┘
```

**Fungsi Tersedia (38 fungsi):**

| ID | Nama | Arg | Return | Deskripsi |
|----|------|-----|--------|-----------|
| 1 | setFuelAdd | 1 | No | Fuel adjustment (%) |
| 2 | setFuelMult | 1 | No | Fuel multiplier |
| 3 | setTimingAdd | 1 | No | Timing adjustment (°) |
| 4 | setTimingMult | 1 | No | Timing multiplier |
| 5 | setBoostTargetAdd | 1 | No | Boost target adder (kPa) |
| 6 | setBoostTargetMult | 1 | No | Boost target multiplier |
| 7 | setBoostDutyAdd | 1 | No | Boost duty adder (%) |
| 8 | setIdleAdd | 1 | No | Idle position adder (%) |
| 9 | setIdleRpm | 1 | No | Idle RPM target |
| 10 | getEtbTarget | 0 | Yes | ETB target position (%) |
| 11 | setEtbAdd | 1 | No | ETB adder (%) |
| 12 | setEwgAdd | 1 | No | Electronic wastegate adder (%) |
| 13 | setEtbDisabled | 1 | No | Disable ETB (0=enable) |
| 14 | setIgnDisabled | 1 | No | Disable ignition (0=enable) |
| 15 | setFuelDisabled | 1 | No | Disable fuel (0=enable) |
| 22 | getGpPwm | 1 | Yes | GPPWM output by index |
| 28 | selfStimulateRPM | 1 | No | Trigger simulator RPM |
| 32 | getIdlePosition | 0 | Yes | Current idle position (%) |
| 33 | getTorque | 0 | Yes | Torque estimate (Nm) |
| 36 | getEngineState | 0 | Yes | Engine state (0=stop,1=crank,2=run) |
| 38 | setLuaGauge | 2 | No | Set Lua gauge (value, index) |

### 4.5. Data Types & Byte Order

Semua multi-byte menggunakan **big-endian** (network byte order).

| Tipe | Ukuran | Range | Contoh (big-endian) |
|------|--------|-------|---------------------|
| int32 | 4 byte | -2³¹ s/d 2³¹-1 | `0x12345678` → `[12 34 56 78]` |
| float32 | 4 byte | IEEE 754 | `123.45` → `[42 F6 E6 66]` |
| uint16 | 2 byte | 0 s/d 65535 | `0x000A` → `[00 0A]` |
| int16 | 2 byte | -32768 s/d 32767 | `-1` → `[FF FF]` |

**Big-endian helper functions** (dari firmware):
```cpp
// int32 → 4 bytes big-endian
void writeInt32BigEndian(int32_t value, unsigned char* out) {
    out[0] = (value >> 24) & 0xFF;
    out[1] = (value >> 16) & 0xFF;
    out[2] = (value >> 8) & 0xFF;
    out[3] = value & 0xFF;
}

// float32 → 4 bytes big-endian (via union type-punning)
void writeFloat32BigEndian(float value, unsigned char* out) {
    union { float f; uint32_t u; } conv = { .f = value };
    out[0] = (conv.u >> 24) & 0xFF;
    out[1] = (conv.u >> 16) & 0xFF;
    out[2] = (conv.u >> 8) & 0xFF;
    out[3] = conv.u & 0xFF;
}

// 4 bytes big-endian → int32
int32_t readInt32BigEndian(const unsigned char* in) {
    return ((int32_t)in[0] << 24) |
           ((int32_t)in[1] << 16) |
           ((int32_t)in[2] << 8)  |
           ((int32_t)in[3]);
}

// 4 bytes big-endian → float32
float readFloat32BigEndian(const unsigned char* in) {
    union { float f; uint32_t u; } conv = {
        .u = ((uint32_t)in[0] << 24) |
             ((uint32_t)in[1] << 16) |
             ((uint32_t)in[2] << 8)  |
             ((uint32_t)in[3])
    };
    return conv.f;
}
```

### 4.6. CAN Hardware Filter

MCP2515 dikonfigurasi untuk **hanya menerima CAN ID `0x721`** (variable response):

```cpp
CAN.setFilterMask(MASK0, false, 0x7FF);  // Match all 11 bits
CAN.setFilterMask(MASK1, false, 0x7FF);

CAN.setFilter(RXF0, false, 0x721);  // CAN_ID_VAR_RESPONSE
CAN.setFilter(RXF1, false, 0x721);
CAN.setFilter(RXF2, false, 0x721);
CAN.setFilter(RXF3, false, 0x721);
CAN.setFilter(RXF4, false, 0x721);
CAN.setFilter(RXF5, false, 0x721);
```

Ini mengurangi CPU overhead dengan memfilter frame yang tidak diinginkan di level hardware.

---

## 5. FIRMWARE — STRUKTUR KODE

### 5.1. File Mapping

| File | Baris | Isi |
|------|-------|-----|
| `mega_epic_canbus.ino` | 940 | Firmware utama: semua logika I/O, CAN TX/RX, state machine |
| `nmea_parser.h` | 51 | Header: struct GPSData, API deklarasi |
| `nmea_parser.cpp` | 469 | Implementasi parser NMEA-0183 |
| `variables.json` | 88 | Mapping nama variabel → CRC32 hash |

### 5.2. Konfigurasi & Constants

Semua konstanta penting didefinisikan di awal file firmware:

```cpp
// CAN Configuration
#define BOARD_CAN_CLOCK   MCP_16MHZ     // 16MHz for Seeed Studio shield
#define SPI_CS_PIN        9             // MCP2515 chip select
#define ECU_CAN_ID        1             // Device address (0-15)

// CAN IDs (derived from ECU_CAN_ID)
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)   // 0x701
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)   // 0x721
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)   // 0x741
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)   // 0x761
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)   // 0x781

// Timing
#define SLOW_OUT_REQUEST_INTERVAL_MS  25   // Output poll interval
#define TX_INTERVAL_FAST_MS           25   // Fast TX (changed)
#define TX_INTERVAL_SLOW_MS          500   // Slow TX (stable/heartbeat)
#define TX_READ_INTERVAL_MS           10   // Input sampling rate

// Change Detection Thresholds
#define TX_ANALOG_THRESHOLD  2.0f   // ADC counts
#define TX_VSS_THRESHOLD     0.1f   // Pulses per second

// VSS Configuration
#define VSS_CALC_INTERVAL_MS  25   // Rate calculation interval
#define VSS_ENABLE_PULLUP      1    // Enable internal pullups

// GPS Configuration
#define GPS_BAUD_RATE         115200
#define GPS_UPDATE_RATE_HZ    20
```

### 5.3. Helper Functions (Endian Conversion)

Empat fungsi untuk konversi big-endian ↔ native:

| Fungsi | Baris | Tujuan |
|--------|-------|--------|
| `writeInt32BigEndian()` | 189-195 | int32 → 4 byte big-endian |
| `writeFloat32BigEndian()` | 197-205 | float32 → 4 byte big-endian (union type-punning) |
| `readInt32BigEndian()` | 207-215 | 4 byte big-endian → int32 |
| `readFloat32BigEndian()` | 217-225 | 4 byte big-endian → float32 |

### 5.4. CAN Frame Helpers

| Fungsi | Baris | Tujuan |
|--------|-------|--------|
| `sendVariableSetFrame(hash, floatVal)` | 230-242 | Kirim variable_set dengan nilai float32 |
| `sendVariableSetFrameU32(hash, uint32)` | 244-256 | Kirim variable_set dengan nilai uint32 (untuk packed data) |
| `sendVariableRequestFrame(hash)` | 258-269 | Kirim variable_request untuk meminta nilai dari ECU |

Semua fungsi menggunakan static `can_frame txMsg` untuk menghindari alokasi stack/heap churn.

---

## 6. MODUL INPUT

### 6.1. Analog Input (A0–A15)

**Konfigurasi Pin:**
```cpp
for (uint8_t i = 0; i < 16; ++i) {
    pinMode(A0 + i, INPUT_PULLUP);
}
```

**Pembacaan (setiap 10ms):**
```cpp
void readAnalogInputs(float* values) {
    for (uint8_t i = 0; i < 16; ++i) {
        values[i] = (float)analogRead(A0 + i);  // 0-1023
    }
}
```

**Variable Hash Mapping:**
```cpp
const int32_t VAR_HASH_ANALOG[16] = {
    595545759,   // A0  → MEGA_EPIC_1_A0
    595545760,   // A1  → MEGA_EPIC_1_A1
    595545761,   // A2  → MEGA_EPIC_1_A2
    595545762,   // A3  → MEGA_EPIC_1_A3
    595545763,   // A4  → MEGA_EPIC_1_A4
    595545764,   // A5  → MEGA_EPIC_1_A5
    595545765,   // A6  → MEGA_EPIC_1_A6
    595545766,   // A7  → MEGA_EPIC_1_A7
    595545767,   // A8  → MEGA_EPIC_1_A8
    595545768,   // A9  → MEGA_EPIC_1_A9
   -1821826352,  // A10 → MEGA_EPIC_1_A10
   -1821826351,  // A11 → MEGA_EPIC_1_A11
   -1821826350,  // A12 → MEGA_EPIC_1_A12
   -1821826349,  // A13 → MEGA_EPIC_1_A13
   -1821826348,  // A14 → MEGA_EPIC_1_A14
   -1821826347   // A15 → MEGA_EPIC_1_A15
};
```

**Change Detection:**
```cpp
bool hasAnalogChanged(uint8_t channel, float newValue) {
    float diff = newValue - analogTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff;  // absolute value
    return (diff >= TX_ANALOG_THRESHOLD);  // threshold: 2.0 ADC counts
}
```

Threshold 2.0 ADC counts ≈ 10mV pada referensi 5V (1024 step / 5V × 2 = ~10mV). Ini mencegah noise triggering transmisi yang tidak perlu.

### 6.2. Digital Input (D22–D37)

**Konfigurasi Pin:**
```cpp
for (uint8_t pin = 22; pin <= 37; ++pin) {
    pinMode(pin, INPUT_PULLUP);
}
```

**Pembacaan (setiap 10ms) — bitfield 16-bit:**
```cpp
void readDigitalInputs(uint16_t* bits) {
    *bits = 0;
    for (uint8_t pin = 22; pin <= 37; ++pin) {
        uint8_t bitIndex = pin - 22;  // D22 → bit0, D23 → bit1, ..., D37 → bit15
        int state = digitalRead(pin);
        if (state == LOW) {           // Inverted logic: grounded = 1
            *bits |= (1u << bitIndex);
        }
    }
}
```

**Logika Inverted:** Karena menggunakan INPUT_PULLUP, pin floating akan HIGH (tidak aktif). Ketika pushbutton ditekan ke GND, pin menjadi LOW (aktif/tertrigger).

**Variable Hash:**
```cpp
const int32_t VAR_HASH_D22_D37 = 2138825443;  // MEGA_EPIC_1_D20_D34 di variables.json
```

**⚠️ KETIDAKSESUAIAN:** Firmware menggunakan hash `2138825443` untuk D22-D37, tapi `variables.json` mendefinisikan `MEGA_EPIC_1_D20_D34` dengan hash `2136453598`. Perlu verifikasi mana yang benar.

**Transmisi:**
```cpp
transmitIfNeeded(&digitalTxState, VAR_HASH_D22_D37, (float)currentDigitalBits, nowMs);
```
Nilai uint16 di-cast ke float32 untuk transmisi CAN.

### 6.3. VSS Wheel Speed (D18–D21)

**Konsep:** 4 sensor kecepatan roda (untuk traction control/ABS) menggunakan external interrupt.

**Pin Mapping:**
| Wheel | Pin | Interrupt | ISR Function |
|-------|-----|-----------|-------------|
| FrontLeft | D18 | INT3 | `vssFrontLeftISR()` |
| FrontRight | D19 | INT2 | `vssFrontRightISR()` |
| RearLeft | D20 | INT1 | `vssRearLeftISR()` |
| RearRight | D21 | INT0 | `vssRearRightISR()` |

**Data Structure:**
```cpp
struct VSSChannel {
    volatile uint32_t edgeCount;   // Edge counter (incremented by ISR)
    uint32_t lastCount;            // Snapshot for rate calculation
    unsigned long lastCalcTime;    // Timestamp of last calculation
    float pulsesPerSecond;         // Calculated rate
};

VSSChannel vssChannels[4] = {
    {0, 0, 0, 0.0f},  // FrontLeft
    {0, 0, 0, 0.0f},  // FrontRight
    {0, 0, 0, 0.0f},  // RearLeft
    {0, 0, 0, 0.0f}   // RearRight
};
```

**ISR Implementation (minimal — hanya increment):**
```cpp
void vssFrontLeftISR() {
    if (vssChannels[0].edgeCount < 0xFFFFFFFE) {
        vssChannels[0].edgeCount++;
    }
}
```
Guard `0xFFFFFFFE` mencegah overflow ke 0xFFFFFFFF.

**Konfigurasi Interrupt (setup):**
```cpp
// Disable I2C to free D20/D21
TWCR &= ~(1<<TWEN);

// Configure pins with pullup
pinMode(VSS_FRONT_LEFT_PIN, INPUT_PULLUP);
pinMode(VSS_FRONT_RIGHT_PIN, INPUT_PULLUP);
pinMode(VSS_REAR_LEFT_PIN, INPUT_PULLUP);
pinMode(VSS_REAR_RIGHT_PIN, INPUT_PULLUP);

// Attach falling edge interrupts
attachInterrupt(digitalPinToInterrupt(VSS_FRONT_LEFT_PIN), vssFrontLeftISR, FALLING);
attachInterrupt(digitalPinToInterrupt(VSS_FRONT_RIGHT_PIN), vssFrontRightISR, FALLING);
attachInterrupt(digitalPinToInterrupt(VSS_REAR_LEFT_PIN), vssRearLeftISR, FALLING);
attachInterrupt(digitalPinToInterrupt(VSS_REAR_RIGHT_PIN), vssRearRightISR, FALLING);
```

**Rate Calculation (setiap 25ms):**
```cpp
void calculateVSSRates() {
    static unsigned long lastCalcTime = 0;
    unsigned long now = millis();
    unsigned long timeDelta = now - lastCalcTime;

    if (timeDelta >= VSS_CALC_INTERVAL_MS && timeDelta < 1000) {
        float timeDeltaSeconds = timeDelta / 1000.0f;
        for (uint8_t i = 0; i < 4; ++i) {
            uint32_t currentCount = vssChannels[i].edgeCount;
            uint32_t countDelta = currentCount - vssChannels[i].lastCount;
            vssChannels[i].pulsesPerSecond = countDelta / timeDeltaSeconds;
            vssChannels[i].lastCount = currentCount;
        }
        lastCalcTime = now;
    }
    else if (timeDelta >= 1000) {
        // Overflow reset
        lastCalcTime = now;
        for (uint8_t i = 0; i < 4; ++i) {
            vssChannels[i].lastCount = vssChannels[i].edgeCount;
            vssChannels[i].pulsesPerSecond = 0.0f;
        }
    }
}
```

**Formula:** `pulsesPerSecond = countDelta / timeDeltaSeconds`

Misal: 100 edge dalam 25ms → 100 / 0.025 = 4000 pulses/second

**Overflow Handling:**
- `millis()` overflow setiap ~49.7 hari → timeDelta tiba-tiba besar → reset
- Counter overflow: unsigned subtraction `currentCount - lastCount` handle wrap-around dengan benar
- Guard `0xFFFFFFFE` mencegah counter mencapai 0xFFFFFFFF (yang akan menyebabkan delta = 0)

**Variable Hashes:**
```cpp
const int32_t VAR_HASH_VSS_FRONT_LEFT  = -1645222329;  // canVSSFrontLeft
const int32_t VAR_HASH_VSS_FRONT_RIGHT = 1549498074;   // canVSSFrontRight
const int32_t VAR_HASH_VSS_REAR_LEFT   = 768443592;    // canVSSRearLeft
const int32_t VAR_HASH_VSS_REAR_RIGHT  = -403905157;   // canVSSRearRight
```

### 6.4. GPS Module (Serial2)

**Konfigurasi Serial:**
```cpp
#define GPS_SERIAL Serial2
#define GPS_BAUD_RATE 115200
#define GPS_UPDATE_RATE_HZ 20

GPS_SERIAL.begin(GPS_BAUD_RATE);
```

**Alur GPS:**
1. GPS module mengirim NMEA-0183 sentences via Serial2
2. `readGPSData()` membaca karakter per karakter, feed ke `nmeaParserProcessChar()`
3. Parser mengekstrak data ke struct `GPSData`
4. `transmitGPSIfNeeded()` membandingkan dengan data terakhir, kirim via smart TX

**Data Structure:**
```cpp
struct GPSData {
    uint8_t hours, minutes, seconds;     // Waktu UTC
    uint8_t days, months, years;         // Tanggal (year 2-digit: 0-99)
    uint8_t quality;                     // Fix quality (0=no fix, 1=GPS, 2=DGPS)
    uint8_t satellites;                  // Jumlah satellite
    float accuracy;                      // HDOP (lower = better)
    float altitude;                      // Altitude (meters)
    float course;                        // Course over ground (degrees)
    float latitude;                      // Decimal degrees (-90 to 90)
    float longitude;                     // Decimal degrees (-180 to 180)
    float speed;                         // km/h (dikonversi dari knots)
    bool hasFix;                         // GPS fix valid
    bool dataValid;                      // Pernah menerima data valid
};
```

**GPS → CAN Packing:**

Dua variabel dipacking ke uint32_t untuk menghemat bandwidth CAN:

```
gps_hmsd_packed (hash: 703958849):
┌──────┬──────┬──────┬──────┐
│ hours│minutes│seconds│ days │
│ 1byte│ 1byte│ 1byte │ 1byte│
└─byte0┴─byte1┴─byte2 ┴─byte3┘
  (LSB)                    (MSB)

gps_myqsat_packed (hash: -1519914092):
┌────────┬──────┬────────┬──────────┐
│ months │ years│ quality│satellites│
│ 1byte  │ 1byte│ 1byte  │  1byte   │
└──byte0 ┴─byte1┴──byte2 ┴───byte3  ┘
```

Variable individual (dikirim sebagai float32):
- `gps_accuracy` (hash: -1489698215)
- `gps_altitude` (hash: -2100224086)
- `gps_course` (hash: 1842893663)
- `gps_latitude` (hash: 1524934922)
- `gps_longitude` (hash: -809214087)
- `gps_speed` (hash: -1486968225)

**Conditional Transmission:**
```cpp
if (gpsData.hasFix) {
    // Only transmit position, altitude, accuracy, course, speed if fix is valid
    transmitGPSIfNeeded();
}
```

---

## 7. MODUL OUTPUT

### 7.1. Slow GPIO (D39–D43, D47–D49)

**Konfigurasi Pin:**
```cpp
const uint8_t SLOW_GPIO_PINS[8] = {39, 40, 41, 42, 43, 47, 48, 49};

for (uint8_t i = 0; i < 8; ++i) {
    pinMode(SLOW_GPIO_PINS[i], OUTPUT);
    digitalWrite(SLOW_GPIO_PINS[i], LOW);  // Start LOW
}
```

**Mekanisme (Polling):**

Setiap 25ms, Mega mengirim `variable_request` untuk `VAR_HASH_OUT_SLOW`:
```cpp
sendVariableRequestFrame(VAR_HASH_OUT_SLOW);  // 1430780106
```

ECU merespon dengan `variable_response` yang berisi uint32_t bitfield.

**Bitfield Layout (ECU-side):**
```
Bit 0: MEGA_EPIC_1_SLOW_D39
Bit 1: MEGA_EPIC_1_SLOW_D40
Bit 2: MEGA_EPIC_1_SLOW_D41
Bit 3: MEGA_EPIC_1_SLOW_D42
Bit 4: MEGA_EPIC_1_SLOW_D43
Bit 5: MEGA_EPIC_1_SLOW_D47
Bit 6: MEGA_EPIC_1_SLOW_D48
Bit 7: MEGA_EPIC_1_SLOW_D49
```

**RX Handler:**
```cpp
if (rxMsg.can_id == CAN_ID_VAR_RESPONSE && rxMsg.can_dlc == 8) {
    int32_t hash = readInt32BigEndian(&rxMsg.data[0]);
    if (hash == VAR_HASH_OUT_SLOW) {
        float value = readFloat32BigEndian(&rxMsg.data[4]);
        uint32_t rawBits = (value >= 0.0f) ? (uint32_t)(value + 0.5f) : 0u;

        // Slow GPIO bits (0-7)
        uint8_t slowBits = (uint8_t)(rawBits & 0xFFu);
        for (uint8_t i = 0; i < 8; ++i) {
            digitalWrite(SLOW_GPIO_PINS[i], (slowBits & (1u << i)) ? HIGH : LOW);
        }
    }
}
```

### 7.2. PWM Output (D3, D5–D8, D11, D12, D44–D46)

**Konfigurasi Pin:**
```cpp
const uint8_t PWM_OUTPUT_PINS[10] = {3, 5, 6, 7, 8, 11, 12, 44, 45, 46};

for (uint8_t i = 0; i < 10; ++i) {
    pinMode(PWM_OUTPUT_PINS[i], OUTPUT);
    analogWrite(PWM_OUTPUT_PINS[i], 0);  // Start 0% duty cycle
}
```

**Bitfield Layout (ECU-side, bit 8-17 dari variabel yang sama dengan slow GPIO):**
```
Bit 8:  MEGA_EPIC_1_PWM_T3_D3    (Timer3)
Bit 9:  MEGA_EPIC_1_PWM_T3_D5    (Timer3)
Bit 10: MEGA_EPIC_1_PWM_T4_D6    (Timer4)
Bit 11: MEGA_EPIC_1_PWM_T4_D7    (Timer4)
Bit 12: MEGA_EPIC_1_PWM_T4_D8    (Timer4)
Bit 13: MEGA_EPIC_1_PWM_T1_D11   (Timer1)
Bit 14: MEGA_EPIC_1_PWM_T1_D12   (Timer1)
Bit 15: MEGA_EPIC_1_PWM_T5_D44   (Timer5)
Bit 16: MEGA_EPIC_1_PWM_T5_D45   (Timer5)
Bit 17: MEGA_EPIC_1_PWM_T5_D46   (Timer5)
```

**⚠️ KETERBATASAN:** Saat ini hanya on/off (0 atau 255). Belum implementasi duty cycle sesungguhnya.
```cpp
if (rawBits & (1u << (i + 8))) {
    analogWrite(pin, 255);  // 100% ON
} else {
    analogWrite(pin, 0);    // 0% OFF
}
```

---

## 8. SMART TRANSMISSION SYSTEM

### 8.1. Konsep

Smart Transmission mengurangi beban CAN bus dengan mengirim data lebih cepat ketika nilai berubah, dan lebih lambat (heartbeat) ketika nilai stabil.

**Mengapa?** Tanpa smart TX, semua 29 channel akan transmit setiap 10ms = 2900 frame/detik — melebihi kapasitas CAN bus 500 kbps (~800 fps).

### 8.2. State Machine

Setiap channel memiliki state machine independent:

```
                    ┌──────────────────┐
       perubahan    │                  │   TX setiap 25ms
       terdeteksi──►│   TX_STATE_      │◄── samapi nilai
                    │   CHANGED (0)    │    tidak berubah
                    │                  │    selama 25ms
                    └────────┬─────────┘
                             │
                  25ms tanpa perubahan
                             │
                             ▼
                    ┌──────────────────┐
                    │   TX_STATE_      │   TX setiap 500ms
                    │   STABLE (1)     │   (heartbeat)
                    │                  │
                    └──────────────────┘
                             │
                   perubahan baru ──► kembali ke CHANGED
```

### 8.3. Data Structures

```cpp
struct TxChannelState {
    float lastTransmittedValue;    // Nilai terakhir yang dikirim
    unsigned long lastTxTime;      // Timestamp (ms) TX terakhir
    bool hasChanged;              // Ada perubahan sejak TX terakhir
    uint8_t state;                // TX_STATE_CHANGED atau TX_STATE_STABLE
};

// Total 29 channel
TxChannelState analogTxState[16];  // 16 analog
TxChannelState digitalTxState;     // 1 digital bitfield
TxChannelState vssTxState[4];      // 4 VSS
TxChannelState gpsTxState[8];      // 8 GPS variables
```

**State Machine Functions:**

```cpp
void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs) {
    if (changed) {
        state->hasChanged = true;
        state->state = TX_STATE_CHANGED;
    } else {
        state->hasChanged = false;
        // Transisi ke STABLE setelah fast interval elapsed
        if (state->state == TX_STATE_CHANGED &&
            (nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS) {
            state->state = TX_STATE_STABLE;
        }
    }
}

bool shouldTransmit(TxChannelState* state, unsigned long nowMs) {
    if (state->hasChanged) {
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS);  // 25ms
    } else {
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_SLOW_MS);  // 500ms
    }
}

void transmitIfNeeded(TxChannelState* state, int32_t varHash, float value, unsigned long nowMs) {
    if (shouldTransmit(state, nowMs)) {
        sendVariableSetFrame(varHash, value);
        state->lastTransmittedValue = value;
        state->lastTxTime = nowMs;
        state->hasChanged = false;
        if (state->state == TX_STATE_CHANGED) {
            state->state = TX_STATE_STABLE;
        }
    }
}
```

### 8.4. Thresholds

| Channel | Threshold | Alasan |
|---------|-----------|--------|
| Analog | ±2.0 ADC counts (~10mV) | Noise filtering |
| Digital | Exact match (0 atau 1) | Bitfield harus presisi |
| VSS | ±0.1 pulses/second | Fluktuasi kecil diabaikan |
| GPS float | ±0.001 | Presisi koordinat yang wajar |

### 8.5. CAN Bus Load Analysis

**Frame size:** ~130 µs per frame (standard CAN 2.0A, 8 byte data, 500 kbps)

| Skenario | Channel Changed | Channel Stable | Total fps | Bus Load |
|----------|----------------|----------------|-----------|----------|
| **Best case** (semua stabil) | 0 | 29 | 29 × 2 = 58 fps | 7.5 ms = 0.75% |
| **Typical** (beberapa berubah) | 10 | 19 | (10×40)+(19×2) = 438 fps | 57 ms = 5.7% |
| **Worst case** (semua berubah) | 21 (non-GPS) | 8 (GPS) | (21×40)+(8×2) = 856 fps | 111 ms = 11.1% |
| **Overload** (melebihi kapasitas) | — | — | >1000 fps | >130 ms = 13% |

**Kesimpulan:** Pada skenario typical, beban CAN bus aman (~5.7%). Worst case mendekati batas teoritis 800 fps untuk 500 kbps, tapi masih dalam batas praktis 400-700 fps Mega2560.

---

## 9. NMEA PARSER (GPS)

### 9.1. Supported Sentences

| Sentence | Format | Data Diekstrak |
|----------|--------|---------------|
| `$GPRMC` | Recommended Minimum Course | Time, date, position, speed, course, status |
| `$GPGGA` | Global Positioning System Fix Data | Quality, satellites, HDOP, altitude, position |

### 9.2. Alur Parsing

```
Serial2 → karakter per karakter
              │
              ▼
         ┌─────────┐
    '$'  │ Start   │──► reset buffer, mulai collect
         └─────────┘
              │
              ▼
         ┌─────────┐
  '\r'/'n'│ End     │──► null-terminate, parse sentence
         └─────────┘
              │
       ┌──────┴──────┐
       ▼              ▼
  ┌─────────┐   ┌─────────┐
  │ $GPRMC  │   │ $GPGGA  │
  │ parse() │   │ parse() │
  └────┬────┘   └────┬────┘
       │              │
       └──────┬───────┘
              ▼
       ┌──────────┐
       │ GPSData  │
       │ struct   │
       └──────────┘
```

**Implementasi parser** (nmea_parser.cpp):
```cpp
bool nmeaParserProcessChar(char c, struct GPSData* gpsData) {
    if (c == '$') {
        nmeaBufferIndex = 0;
        nmeaBuffer[0] = c;
        nmeaBufferIndex = 1;
        return false;
    }
    else if (c == '\r' || c == '\n') {
        if (nmeaBufferIndex > 0) {
            nmeaBuffer[nmeaBufferIndex] = '\0';
            bool parsed = false;
            if (strncmp(nmeaBuffer, "$GPRMC", 6) == 0)
                parsed = parseGPRMC(nmeaBuffer, gpsData);
            else if (strncmp(nmeaBuffer, "$GPGGA", 6) == 0)
                parsed = parseGPGGA(nmeaBuffer, gpsData);
            nmeaBufferIndex = 0;
            return parsed;
        }
        return false;
    }
    else if (nmeaBufferIndex < NMEA_SENTENCE_MAX_LEN) {
        nmeaBuffer[nmeaBufferIndex++] = c;
        return false;
    }
    else {
        nmeaBufferIndex = 0;  // Buffer overflow — reset
        return false;
    }
}
```

### 9.3. Coordinate Conversion

NMEA menggunakan format **DDMM.MMMM** (latitude) dan **DDDMM.MMMM** (longitude).

**Latitude Example:** `3739.8560,S` = 37° 39.8560' South
```
Degrees: 37
Minutes: 39.8560
Decimal: 37 + (39.8560 / 60.0) = 37.66427°
South → negative: -37.66427°
```

```cpp
float parseLatitudeFromNMEA(const char* latStr, char ns) {
    char degStr[3] = {latStr[0], latStr[1], '\0'};  // First 2 chars = degrees
    int degrees = atoi(degStr);
    float minutes = atof(latStr + 2);  // Rest = minutes
    float decimalDegrees = degrees + (minutes / 60.0f);
    if (ns == 'S' || ns == 's') decimalDegrees = -decimalDegrees;
    return decimalDegrees;
}
```

**Longitude Example:** `11550.3525,E` = 115° 50.3525' East
```
Degrees: 115 (first 3 chars)
Minutes: 50.3525
Decimal: 115 + (50.3525 / 60.0) = 115.83921°
```

```cpp
float parseLongitudeFromNMEA(const char* lonStr, char ew) {
    char degStr[4] = {lonStr[0], lonStr[1], lonStr[2], '\0'};  // First 3 chars
    int degrees = atoi(degStr);
    float minutes = atof(lonStr + 3);
    float decimalDegrees = degrees + (minutes / 60.0f);
    if (ew == 'W' || ew == 'w') decimalDegrees = -decimalDegrees;
    return decimalDegrees;
}
```

### 9.4. GPS → CAN Packing

**Packing HMSD (Hours, Minutes, Seconds, Days):**
```cpp
uint32_t packGPSHMSD(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t days) {
    return ((uint32_t)hours) |
           ((uint32_t)minutes << 8) |
           ((uint32_t)seconds << 16) |
           ((uint32_t)days << 24);
}
```

**Packing MYQSAT (Months, Years, Quality, Satellites):**
```cpp
uint32_t packGPSMYQSAT(uint8_t months, uint8_t years, uint8_t quality, uint8_t satellites) {
    return ((uint32_t)months) |
           ((uint32_t)years << 8) |
           ((uint32_t)quality << 16) |
           ((uint32_t)satellites << 24);
}
```

---

## 10. TIMING & PERFORMANCE

### 10.1. Timing Diagram

```
Timeline (ms)
0──────10──────20──────30──────40──────50──────60──────70──────80──────90──────100
│      │      │      │      │      │      │      │      │      │      │
├──────┴──────┴──────┴──────┴──────┤
│      CAN RX Polling (continuous)  │
│      (setiap iterasi loop)        │
│                                   │
├──────┤     ├──────┤     ├──────┤  │
│ Read │     │ Read │     │ Read │  │  Input Read (10ms)
│ I/O  │     │ I/O  │     │ I/O  │  │
│      │     │      │     │      │  │
├──────┤     ├──────┤     ├──────┤  │
│ VSS  │     │ VSS  │     │ VSS  │  │  VSS Calc (25ms)
│ calc │     │ calc │     │ calc │  │
│      │     │      │     │      │  │
├──────┤     ├──────┤     ├──────┤  │
│OutReq│     │OutReq│     │OutReq│  │  Output Request (25ms)
│      │     │      │     │      │  │
│├┤├┤│├┤├┤│├┤├┤│├┤├┤│├┤├┤│├┤├┤│├┤  │  Smart TX (setiap loop)
││││││││││││││││││││││││││││││││  │  — hanya jika need TX
└─────────────────────────────────────┘
```

### 10.2. CAN Bus Load Calculation

**Frame timing CAN 2.0A (standard, 500 kbps):**
```
Bit timing:
  SOF:    1 bit
  ID:    11 bit
  RTR:    1 bit
  IDE:    1 bit
  r0:     1 bit
  DLC:    4 bit
  Data:   8 byte × 8 = 64 bit
  CRC:   15 bit
  CRCdel: 1 bit
  ACK:    2 bit
  EOF:    7 bit
  IFS:    3 bit
  ──────────────────
  Total: 111 bit = 111 / 500000 = 222 µs ≈ 0.222 ms per frame
```

**Butir pembahasan:** 0.222 ms per frame → ~4500 fps theoretical max.
Tapi karena SPI overhead, CPU processing, dan buffer MCP2515, praktis hanya 400-700 fps.

### 10.3. Memory Usage

**SRAM Usage Estimate:**

| Komponen | Ukuran |
|----------|--------|
| VSS Channel struct × 4 | 4 × (4+4+4+4) = 64 bytes |
| TxChannelState analog × 16 | 16 × (4+4+1+1) = ~160 bytes |
| TxChannelState digital × 1 | 4+4+1+1 = ~10 bytes |
| TxChannelState VSS × 4 | 4 × ~10 = ~40 bytes |
| TxChannelState GPS × 8 | 8 × ~10 = ~80 bytes |
| GPSData struct | ~30 bytes |
| GPS data copy | ~30 bytes |
| NMEA buffer | 83 bytes |
| Current analog values × 16 | 64 bytes |
| Current VSS values × 4 | 16 bytes |
| Current digital bits | 2 bytes |
| CAN frame struct | ~10 bytes |
| **Subtotal** | **~589 bytes** |
| Stack & overhead | ~1 KB |
| Library globals | ~1 KB |
| **Total SRAM** | **~2.6 KB** dari 8 KB |

Mashi ada ~5.4 KB untuk runtime dan buffer tambahan. Aman.

---

## 11. VARIABLE HASH MAPPING

Semua hash adalah CRC32-based signed int32. Hash konsisten antara firmware Mega dan ECU epicEFI.

### 11.1. Analog Inputs

| Pin | Nama Variabel | Hash (int32) | Hash (hex) |
|-----|--------------|--------------|------------|
| A0 | MEGA_EPIC_1_A0 | 595545759 | 0x237BB91F |
| A1 | MEGA_EPIC_1_A1 | 595545760 | 0x237BB920 |
| A2 | MEGA_EPIC_1_A2 | 595545761 | 0x237BB921 |
| A3 | MEGA_EPIC_1_A3 | 595545762 | 0x237BB922 |
| A4 | MEGA_EPIC_1_A4 | 595545763 | 0x237BB923 |
| A5 | MEGA_EPIC_1_A5 | 595545764 | 0x237BB924 |
| A6 | MEGA_EPIC_1_A6 | 595545765 | 0x237BB925 |
| A7 | MEGA_EPIC_1_A7 | 595545766 | 0x237BB926 |
| A8 | MEGA_EPIC_1_A8 | 595545767 | 0x237BB927 |
| A9 | MEGA_EPIC_1_A9 | 595545768 | 0x237BB928 |
| A10 | MEGA_EPIC_1_A10 | -1821826352 | 0x934F1390 |
| A11 | MEGA_EPIC_1_A11 | -1821826351 | 0x934F1391 |
| A12 | MEGA_EPIC_1_A12 | -1821826350 | 0x934F1392 |
| A13 | MEGA_EPIC_1_A13 | -1821826349 | 0x934F1393 |
| A14 | MEGA_EPIC_1_A14 | -1821826348 | 0x934F1394 |
| A15 | MEGA_EPIC_1_A15 | -1821826347 | 0x934F1395 |

### 11.2. Gear & Digital Input

| Pin | Nama Variabel | Hash (int32) | Hash (hex) | Bits |
|-----|--------------|--------------|------------|------|
| D22-D26 | detectedGear | **283558758** | **0x10E7C366** | uint8: 0=N,1=G1,2=G2,3=G3,4=G4,255=inv |
| D27-D37 | MEGA_EPIC_1_D20_D34 | 2136453598 | 0x7F5EAD9E | bits 5-15 (11-bit bitfield) |

**detectedGear** — hash resmi dari epicEFI. Nilai: 0=N, 1=G1, 2=G2, 3=G3, 4=G4, 255=invalid.

**MEGA_EPIC_1_D20_D34** — menggunakan hash existing yang sudah ada di epicEFI. Bitfield:
- Bit 0-1: D20-D21 (VSS, selalu 0)
- Bit 2-4: D22-D26 (gear, selalu 0)
- Bit 5-14: D27-D36 (10-bit digital button inputs)
- Bit 15: **D37 (clutch switch)** — switch ke GND saat ditekan

```cpp
// Bit packing: D27-D37 masuk ke bit 5-15
for (uint8_t pin = 27; pin <= 37; ++pin) {
    uint8_t bitIndex = (uint8_t)(pin - 27);  // 0-10
    if (digitalRead(pin) == LOW) {
        bits |= (uint16_t)(1u << (bitIndex + 5));  // Shift ke bit 5-15
    }
}
```

**Tidak perlu hash baru** — reuse hash `MEGA_EPIC_1_D20_D34` yang sudah ada di epicEFI firmware.

**Perubahan dari versi sebelumnya:** D22-D26 (5 pin) dialihkan dari digital input bitfield ke gear selector. Bitfield digital input dikurangi dari 16-bit menjadi 11-bit (D27-D37). Hash diubah dari `2136453598` (MEGA_EPIC_1_D20_D34) menjadi `2138825443` (MEGA_EPIC_1_D27_D37).

### 11.3. VSS Channels

| Wheel | Nama Variabel | Hash (int32) | Hash (hex) |
|-------|--------------|--------------|------------|
| FrontLeft | canVSSFrontLeft | -1645222329 | 0x9E30B7C7 |
| FrontRight | canVSSFrontRight | 1549498074 | 0x5C57131A |
| RearLeft | canVSSRearLeft | 768443592 | 0x2DCD7648 |
| RearRight | canVSSRearRight | -403905157 | 0xE7C22C7B |

### 11.4. GPS Variables

| Nama | Hash (int32) | Hash (hex) | Tipe |
|------|--------------|------------|------|
| gps_hmsd_packed | 703958849 | 0x29F44841 | uint32_t (packed) |
| gps_myqsat_packed | -1519914092 | 0xA57FB014 | uint32_t (packed) |
| gps_accuracy | -1489698215 | 0xA734F719 | float32 |
| gps_altitude | -2100224086 | 0x82D8B9AA | float32 |
| gps_course | 1842893663 | 0x6DD5D3DF | float32 |
| gps_latitude | 1524934922 | 0x5AE00A4A | float32 |
| gps_longitude | -809214087 | 0xCFD27BF9 | float32 |
| gps_speed | -1486968225 | 0xA75864DF | float32 |

### 11.5. Output Variables

| Nama | Hash (int32) | Hash (hex) | Bits |
|------|--------------|------------|------|
| MEGA_EPIC_1_OUT_SLOW | 1430780106 | 0x5544330A | 0-17 (uint32_t) |
| **detectedGear** | **283558758** | **0x10E7C366** | **uint8: 0=N,1=G1,2=G2,3=G3,4=G4,255=inv** |

**Bit allocation:**
- Bit 0-7: Slow GPIO (D39-D43, D47-D49)
- Bit 8-17: PWM on/off (D3, D5-D8, D11, D12, D44-D46)

---

## 12. IMPLEMENTASI DETAIL

### 12.1. Setup Sequence

Urutan inisialisasi di `setup()`:

```
1. Serial.begin(115200)
       │ Debug serial untuk monitoring
       ▼
2. CAN.reset()
   CAN.setBitrate(CAN_500KBPS, MCP_16MHZ)
   CAN.setNormalMode()
       │ Inisialisasi MCP2515
       ▼
3. configureCANFilters()
       │ Filter hardware: hanya terima 0x721
       ▼
4. GPS_SERIAL.begin(GPS_BAUD_RATE)
   nmeaParserInit()
       │ Inisialisasi GPS Serial2 (115200 baud)
       ▼
5. pinMode(A0-A15, INPUT_PULLUP)
       │ 16 analog input dengan pullup
       ▼
6. pinMode(D22-D37, INPUT_PULLUP)
       │ 16 digital input dengan pullup
       ▼
7. pinMode(SLOW_GPIO_PINS, OUTPUT); digitalWrite(..., LOW)
   pinMode(PWM_OUTPUT_PINS, OUTPUT); analogWrite(..., 0)
       │ Output pins: start LOW/OFF
       ▼
8. TWCR &= ~(1<<TWEN)
       │ Disable I2C untuk free D20/D21
       ▼
9. pinMode(VSS pins, INPUT_PULLUP)
   attachInterrupt(VSS pins, ISR, FALLING)
       │ VSS interrupts on D18-D21
       ▼
10. Initialize all TxChannelState
    - lastTransmittedValue = 0
    - lastTxTime = 0
    - hasChanged = true (force initial TX)
    - state = TX_STATE_CHANGED
```

### 12.2. Main Loop Breakdown

Urutan eksekusi setiap iterasi `loop()`:

```
┌─────────────────────────────────────────────┐
│ 1. CAN RX Polling                           │
│    while (CAN.readMessage(&rxMsg) == OK)     │
│        handleCanFrame(rxMsg)                 │
│    ├─ Jika 0x721 & hash=OUT_SLOW            │
│    │      → update slow GPIO dan PWM pins    │
│    └─ Frame lain → discard                   │
├─────────────────────────────────────────────┤
│ 2. VSS Rate Calculation (25ms)              │
│    calculateVSSRates()                       │
│    ├─ Hitung pulsesPerSecond dari edgeCount  │
│    └─ Update lastCount                       │
├─────────────────────────────────────────────┤
│ 3. Output Request (25ms)                    │
│    sendVariableRequestFrame(OUT_SLOW)        │
│    └─ Minta state output dari ECU            │
├─────────────────────────────────────────────┤
│ 4. GPS Read (every loop)                    │
│    readGPSData()                             │
│    └─ Baca Serial2 → parse NMEA              │
├─────────────────────────────────────────────┤
│ 5. Input Read (10ms)                        │
│    readAnalogInputs()                        │
│    readDigitalInputs()                       │
│    readVSSInputs()                           │
│    ├─ Update state machine analog           │
│    ├─ Update state machine digital          │
│    └─ Update state machine VSS              │
├─────────────────────────────────────────────┤
│ 6. Smart TX (every loop)                    │
│    for 16 analog: transmitIfNeeded()        │
│    transmitIfNeeded(digital)                │
│    for 4 VSS: transmitIfNeeded()            │
│    transmitGPSIfNeeded()                     │
│    └─ Kirim frame jika perlu                 │
└─────────────────────────────────────────────┘
```

### 12.3. CAN RX Handler

```cpp
static void handleCanFrame(const struct can_frame& rxMsg) {
    // Hanya proses variable response (0x721) dengan DLC 8
    if ((rxMsg.can_id == CAN_ID_VAR_RESPONSE) && (rxMsg.can_dlc == 8)) {
        int32_t hash = readInt32BigEndian(&rxMsg.data[0]);

        // Output slow GPIO + PWM
        if (hash == VAR_HASH_OUT_SLOW) {
            float value = readFloat32BigEndian(&rxMsg.data[4]);

            // ECU mengirim float, tapi kita treat sebagai uint32_t bitfield
            uint32_t rawBits = (value >= 0.0f) ? (uint32_t)(value + 0.5f) : 0u;

            // Slow GPIO (bit 0-7)
            uint8_t slowBits = rawBits & 0xFF;
            for (uint8_t i = 0; i < 8; ++i) {
                digitalWrite(SLOW_GPIO_PINS[i],
                    (slowBits & (1u << i)) ? HIGH : LOW);
            }

            // PWM (bit 8-17)
            for (uint8_t i = 0; i < 10; ++i) {
                uint8_t bitIndex = i + 8;
                analogWrite(PWM_OUTPUT_PINS[i],
                    (rawBits & (1u << bitIndex)) ? 255 : 0);
            }
        }
        // Hash lain belum diimplementasikan
    }
}
```

**⚠️ Catatan:** Nilai float dari ECU di-cast ke uint32_t untuk mengekstrak bitfield. Ini membutuhkan ECU mengirim nilai integer yang tepat sebagai float. Operasi `(value + 0.5f)` digunakan untuk pembulatan.

### 12.4. VSS Rate Calculation Detail

```cpp
void calculateVSSRates() {
    static unsigned long lastCalcTime = 0;
    unsigned long now = millis();
    unsigned long timeDelta = now - lastCalcTime;

    if (timeDelta >= VSS_CALC_INTERVAL_MS && timeDelta < 1000) {
        // Normal case: calculate rate
        float timeDeltaSeconds = timeDelta / 1000.0f;

        for (uint8_t i = 0; i < 4; ++i) {
            // Atomic read of volatile counter (ISR-safe)
            uint32_t currentCount = vssChannels[i].edgeCount;

            // Unsigned subtraction handles wrap-around correctly
            uint32_t countDelta = currentCount - vssChannels[i].lastCount;

            vssChannels[i].pulsesPerSecond = countDelta / timeDeltaSeconds;
            vssChannels[i].lastCount = currentCount;
        }
        lastCalcTime = now;
    }
    else if (timeDelta >= 1000) {
        // millis() overflow (~49.7 days) — reset
        lastCalcTime = now;
        for (uint8_t i = 0; i < 4; ++i) {
            vssChannels[i].lastCount = vssChannels[i].edgeCount;
            vssChannels[i].pulsesPerSecond = 0.0f;
        }
    }
    // else: too early, skip
}
```

**Overflow millis():**
- `millis()` adalah unsigned long (32-bit), overflow setelah ~49.7 hari
- `timeDelta = now - lastCalcTime` dengan unsigned arithmetic: jika `now < lastCalcTime` (overflow), hasilnya tetap benar untuk selisih < 2³² ms
- Guard `timeDelta < 1000`: jika delta > 1 detik (termasuk kasus overflow), sistem reset

---

## 13. MODUL GEAR DETECTION

Fitur gear position detection di-porting dari `GearIndicatorCan` untuk mendeteksi posisi gigi transmisi manual (N, 1, 2, 3, 4) menggunakan 5 switch input.

### 13.1. Pin Mapping

| Gear | Pin Mega2560 | Logic | Catatan |
|------|-------------|-------|---------|
| Neutral | **D22** | INPUT_PULLUP, active LOW | Grounded = Netral aktif |
| Gear 1 | **D23** | INPUT_PULLUP, active LOW | Grounded = Gigi 1 |
| Gear 2 | **D24** | INPUT_PULLUP, active LOW | Grounded = Gigi 2 |
| Gear 3 | **D25** | INPUT_PULLUP, active LOW | Grounded = Gigi 3 |
| Gear 4 | **D26** | INPUT_PULLUP, active LOW | Grounded = Gigi 4 |

### 13.2. Logika Deteksi

```cpp
static GearState readGearState() {
    uint8_t mask = 0;
    if (digitalRead(GEAR_N_PIN) == LOW) mask |= 0x01;
    if (digitalRead(GEAR_1_PIN) == LOW) mask |= 0x02;
    if (digitalRead(GEAR_2_PIN) == LOW) mask |= 0x04;
    if (digitalRead(GEAR_3_PIN) == LOW) mask |= 0x08;
    if (digitalRead(GEAR_4_PIN) == LOW) mask |= 0x10;

    // Manual popcount — AVR GCC tidak punya __builtin_popcount
    uint8_t count = 0;
    uint8_t m = mask;
    while (m) { count += m & 1; m >>= 1; }

    if (count != 1) return {kInvalid, mask};

    if (mask & 0x01) return {kNeutral, mask};
    if (mask & 0x02) return {kGear1, mask};
    if (mask & 0x04) return {kGear2, mask};
    if (mask & 0x08) return {kGear3, mask};
    return {kGear4, mask};
}
```

### 13.3. Validasi

Hanya **1 dari 5 pin** boleh aktif (LOW) dalam satu waktu. Jika:
- **0 pin aktif** → semua switch open → tidak ada gigi → `kInvalid`
- **2+ pin aktif** → multiple switch error → `kInvalid`
- **Tepat 1 pin aktif** → gigi valid

Ini mencegah false reading dari multiple switch yang tertekan bersamaan.

### 13.4. CAN Transmission

Gear dikirim sebagai variable_set EPIC dengan hash resmi `detectedGear` (`283558758`):

| Nilai | Arti |
|-------|------|
| 0 | Neutral |
| 1 | Gigi 1 |
| 2 | Gigi 2 |
| 3 | Gigi 3 |
| 4 | Gigi 4 |
| 0xFF (255) | Invalid/error |

Menggunakan smart transmission yang sama dengan channel lain: 25ms jika berubah, 500ms heartbeat jika stabil.

---

## 14. AFR DATABOX VIA SERIAL3

Modul DataboxManager membaca data AFR (Air Fuel Ratio) dari wideband controller BRT/Databox melalui Serial3 (D14/D15) pada 57600 baud.

### 14.1. Protocol Databox BRT

Protocol proprietary BRT (Bosch Regulated Tuner) menggunakan UART asynchronous dengan command-response dan streaming data.

**Command Sequence (handshake):**
```
Step 1: Send "3649" (STOP)         → Stop any existing stream
Step 2: Send "3640;00" (START_MAIN)→ Start main data output
Step 3: Send "3648" (START_STREAM) → Start continuous streaming
```

Setelah handshake sukses, controller mengirim frame hex 32 karakter setiap ~10ms.

### 14.2. Frame Format

Frame dimulai dengan signature `"3628"` diikuti 28 karakter hex:

```
Position  Content     Length  Parsing
────────  ──────────  ──────  ─────────────────
[0-3]     Signature   4       "3628" — frame identifier
[4-7]     AFR         4       Hex → uint16 → /100  (contoh: 0E74 = 3700 → 37.00 AFR)
[8-11]    RPM         4       Hex → uint16          (contoh: 05DC = 1500 RPM)
[12-13]   TPS         2       Hex → uint8           (contoh: 3C = 60%)
[14-16]   EOT         3       Hex → uint16 → /10    (contoh: 1F4 = 500 → 50.0°C)
[17-19]   Vbatt       3       Hex → uint16 → /100   (contoh: 10E = 270 → 2.70V)
[20-22]   Ur          3       Hex → uint16 → /1000  (voltage ratio untuk temp probe)
[23-26]   Duty        4       Hex → uint16 → /100   (heater duty cycle %)
[27-28]   Status      2       Raw 2-char string     (contoh: "00" = OK)
[29-31]   UrCal       3       Hex → uint16 → /1000  (calibrated Ur)
```

**Contoh frame:** `36280E7405DC3C1F410E03E8006400001F4`
```
AFR=0E74h/100=37.00, RPM=05DCh=1500, TPS=3Ch=60%, EOT=1F4h/10=50.0°C,
Vbatt=0E0h/100=2.24V, Ur=3E8h/1000=1.000, Duty=0064h/100=1.00%,
Status="00", UrCal=1F4h/1000=0.500
```

### 14.3. State Machine Reconnect

```
IDLE → SEND_STOP → WAIT_STOP (50ms)
                        │
                        ▼
                  SEND_START_MAIN → WAIT_START_MAIN (50ms)
                                        │
                                        ▼
                                  SEND_START_STREAM → WAITING_RETRY (2s timeout)
                                                            │
                                              ┌─────────────┴─────────────┐
                                              ▼                         ▼
                                        CONNECTED              Retry timeout → SEND_STOP
                                              │
                                              │ 5s tanpa data
                                              ▼
                                        SEND_STOP (timeout)
```

**Komponen non-blocking:**
- `CMD_GAP_MS = 50ms` — jeda antar command
- `RECONNECT_RETRY_DELAY = 2000ms` — timeout tunggu streaming
- `DATABOX_TIMEOUT_MS = 5000ms` — timeout data
- `pumpData()` — membaca max 64 byte per iterasi loop

**AFR Validation:** Hanya AFR dalam range 7.0 – 80.0 yang diterima. Nilai di luar range dianggap invalid/sensor belum ready.

### 14.4. Probe Temperature Calculation

Temperatur probe dihitung dari Ur (voltage ratio) menggunakan lookup table dengan interpolasi linear:

```cpp
Resistansi = Ur × 300.0

Lookup Table:
  Res (Ω)  |  2200 |  550  |  300  |  260  |  160  |  80
  Temp (°C)|  600  |  700  |  780  |  800  |  900  | 1000

Interpolasi linear antara dua titik terdekat.
```

Range: 600°C – 1000°C. Di luar range dikembalikan nilai boundary terdekat.

### 14.5. Pin Allocation

| Mega2560 Pin | Fungsi | Koneksi |
|-------------|--------|---------|
| **D14** (TX3) | Serial3 TX → Databox RX | Kirim command ke AFR controller |
| **D15** (RX3) | Serial3 RX ← Databox TX | Terima frame AFR dari controller |
| GND | Ground | Common ground |

**Wiring:**
```
AFR Controller TX → Mega D15 (RX3)
AFR Controller RX ← Mega D14 (TX3)
GND               ←→ GND
```

---

## 15. rusEFI WIDEBAND CAN PROTOCOL

Modul ini mengirim data AFR yang dibaca dari Databox ke CAN bus menggunakan protocol **rusEFI Native Wideband** pada 2 CAN ID: `0x190` dan `0x191`.

### 15.1. Frame 0x190 — Lambda & Temperature

```
DLC: 8
Byte:   [0]     [1]        [2-3]               [4-5]         [6-7]
Field: Version  ValidFlag  Lambda (uint16)      Temp °C (u16) Pad
Order:                     Little Endian        Little Endian
```

| Field | Deskripsi |
|-------|-----------|
| Version `[0]` | `0xA0` — protocol version |
| ValidFlag `[1]` | `0x01` = valid, `0x00` = invalid/sensor off |
| Lambda `[2-3]` | `(AFR / 14.7) × 10000`, LSB first |
| Temp °C `[4-5]` | Probe temperature dari Ur lookup, LSB first |

**Contoh:** AFR 14.7, Temp 780°C
```
[0xA0] [0x01] [0xE8 0x03] [0x0C 0x03] [0x00 0x00]
  Ver    Valid  Lambda=10000  Temp=780    Pad
                (14.7/14.7)   (780=0x030C)
                ×10000=10000
```

### 15.2. Frame 0x191 — Diagnostic

```
DLC: 8
Byte:   [0-1]    [2-3]      [4]       [5]      [6]      [7]
Field:  ESR     NernstDC   PumpDuty  Status  HeaterDuty Reserved
```

| Field | Deskripsi |
|-------|-----------|
| ESR `[0-1]` | Element resistance (dummy 0) |
| NernstDC `[2-3]` | Nernst DC (dummy 0) |
| PumpDuty `[4]` | Pump duty cycle (dummy 0) |
| Status `[5]` | `0x01` = OK/Heating done, `0x00` = Error |
| HeaterDuty `[6]` | Heater duty cycle 0-100% (dari frame Databox) |

### 15.3. Byte Order (Little Endian)

**⚠️ PENTING:** Protocol rusEFI Wideband menggunakan **Little Endian**, BERBEDA dengan EPIC_CAN_BUS yang menggunakan **Big Endian**.

```cpp
// Little Endian: LSB first
frame.data[2] = lambda_val & 0xFF;          // LSB
frame.data[3] = (lambda_val >> 8) & 0xFF;   // MSB

// Bandingkan dengan EPIC Big Endian:
out[0] = (value >> 24) & 0xFF;  // MSB first
out[3] = value & 0xFF;          // LSB last
```

### 15.4. Valid Flag & Error Handling

- `ValidFlag = 0x01` → data AFR valid, ECU wajib memproses
- `ValidFlag = 0x00` → sensor tidak terhubung atau timeout
- Frame `0x191` `Status = 0x00` → error/tidak siap
- Dikirim setiap **10ms** (WIDEBAND_SEND_INTERVAL_MS), sesuai spesifikasi rusEFI `WBO_TX_PERIOD_MS`

Jika Databox tidak terhubung, kedua frame tetap dikirim dengan ValidFlag = 0 dan semua data 0. Ini penting agar ECU tidak menganggap sensor timeout.

---

## 16. CAN ERROR HANDLING

Modul error handling di-porting dari GearIndicatorCan untuk meningkatkan robustness inisialisasi CAN.

### 16.1. Clock Fallback

MCP2515 bisa menggunakan crystal 8MHz atau 16MHz tergantung shield. Firmware mencoba kedua clock:

```cpp
static bool initializeCanController() {
    if (setupCan(MCP_16MHZ)) return true;  // Coba 16MHz dulu
    if (setupCan(MCP_8MHZ)) return true;   // Gagal → fallback 8MHz
    return false;                           // Kedua gagal
}
```

### 16.2. Auto Re-init

Jika inisialisasi CAN gagal, firmware akan mencoba ulang setiap 3 detik:

```cpp
if (!canReady) {
    if ((now - lastCanInitAttemptAt) >= CAN_REINIT_INTERVAL_MS) {
        canReady = initializeCanController();
    }
    delay(100);
    return;  // Skip loop body
}
```

### 16.3. Max Retry & Recovery

Setelah 3 kali percobaan gagal berturut-turut, CAN dianggap rusak permanen. Firmware **tetap berjalan** tanpa CAN — GPS dan Serial3/Databox tetap aktif:

```cpp
if (canInitFailureCycles >= CAN_REINIT_MAX_CYCLES) {
    Serial.println("[CAN] Max retries reached, CAN disabled");
    // Continue without CAN
}
```

Ini berbeda dengan GearIndicatorCan yang melakukan `ESP.restart()`. Mega2560 tidak punya restart otomatis, jadi firmware lanjut dengan fitur non-CAN.

### 16.4. Rate-Limited Logging

Error CAN hanya dicetak setiap `ERROR_LOG_INTERVAL_MS` (2 detik) untuk mencegah spam Serial:

```cpp
static unsigned long lastErrorLogAt = 0;
if ((now - lastErrorLogAt) >= ERROR_LOG_INTERVAL_MS) {
    Serial.print("CAN error flags: 0x");
    Serial.println(CAN0.getErrorFlags(), HEX);
    lastErrorLogAt = now;
}
```

---

## 17. POTENSIAL MASALAH & BUG

### 17.1. Gear Hash Placeholder

**Lokasi:** `mega_epic_canbus.ino` / `variables.json`

Hash `VAR_HASH_GEAR = 283558758` adalah hash resmi untuk variable `detectedGear` dari epicEFI. Sudah terdaftar dan siap digunakan.

**Tidak ada masalah** — hash sudah valid.

### 17.2. Digital Input Bitfield — D20_D34 vs D22_D37

**Lokasi:** `mega_epic_canbus.ino` vs `variables.json`

| Sumber | Nama | Hash | Bit Mapping |
|--------|------|------|-------------|
| Firmware v2 | MEGA_EPIC_1_D20_D34 | 2136453598 | Bits 5-15 (D27-D37), bits 0-4 always 0 |
| epicEFI | MEGA_EPIC_1_D20_D34 | 2136453598 | Bits 0-14 (D20-D34) |

**Issue:** Firmware mengirim D27-D37 di bit 5-15, tapi epicEFI mengharapkan D20-D34 di bit 0-14. Bit 0-4 selalu 0 karena D22-D26 sekarang gear, D20-D21 VSS. ECU tetap menerima data — bit 5-15 berisi nilai digital input D27-D37 yang valid.

**Status:** ✅ Tidak ada masalah — hash reusable, tidak perlu hash baru. ECU menerima 11-bit data meskipun bit 0-4 kosong.

### 17.3. nmeaGetField() Edge Case

**Lokasi:** `nmea_parser.cpp:112-155`

```cpp
const char* nmeaGetField(const char* sentence, uint8_t fieldIndex) {
    while (*p) {
        if (*p == ',' || *p == '*') {
            // ...
        } else {
            p++;
        }
    }
    if (currentField == fieldIndex && fieldStart < p) {
        return fieldStart;
    }
    return NULL;
}
```

**Issue:** Ketika field terakhir tidak memiliki trailing delimiter (`*` atau `,`), pointer `p` bisa melampaui string jika field berada di akhir sentence tanpa checksum. Return pointer bisa ke memory out-of-bounds.

### 17.4. GPS Float Packing Precision

**Lokasi:** `mega_epic_canbus.ino`

`packGPSHMSD()` dan `packGPSMYQSAT()` mengembalikan `uint32_t`, tapi nilai ini disimpan sebagai `float` di `TxChannelState.lastTransmittedValue`:

```cpp
gpsTxState[0].lastTransmittedValue = value;
```

**Issue:** uint32_t > 16,777,216 (2²⁴) tidak bisa direpresentasikan secara presisi oleh float32 IEEE 754. Packed value seperti `0xA57FB014` (2,777,870,356) akan kehilangan presisi saat disimpan sebagai float.

### 17.5. PWM Output Hanya On/Off

**Lokasi:** `mega_epic_canbus.ino`

```cpp
if (rawBits & (1u << bitIndex)) {
    analogWrite(pin, 255);
} else {
    analogWrite(pin, 0);
}
```

**Issue:** PWM output saat ini hanya on/off (bit), bukan duty cycle sebenarnya. Kalau ECU mengirim duty cycle 0-100%, tidak akan diproses dengan benar.

**Rekomendasi:** Implementasi duty cycle penuh menggunakan nilai float dari ECU.

### 17.6. CAN RX Masih Polling

**Lokasi:** `mega_epic_canbus.ino`

```cpp
while (CAN.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
    handleCanFrame(rxMsg);
}
```

**Issue:** Polling membuang CPU cycle dan bisa ketinggalan frame kalau MCP2515 menerima lebih dari 2 frame berturut-turut (buffer hanya 2). Pada CAN traffic tinggi, frame loss hampir pasti.

**Rekomendasi:** Implementasi interrupt-driven CAN RX menggunakan pin INT (D2).

### 17.7. CAN Filter Promiscuous

**Lokasi:** `mega_epic_canbus.ino:configureCANFilters()`

```cpp
CAN.setFilterMask(MASK0, false, 0x000);
```

**Issue:** Karena perlu menerima CAN ID `0x190` (Wideband) dan `0x721` (EPIC response), filter di-set promiscuous (accept all). Ini berarti Mega akan memproses semua frame CAN di bus, meningkatkan CPU load.

**Rekomendasi:** Jika tidak perlu menerima `0x190`, kembalikan filter ke `0x7FF` dengan target `0x721` saja.

### 17.8. Wideband Little Endian vs EPIC Big Endian

**Lokasi:** `mega_epic_canbus.ino:sendWidebandFrame()`

```cpp
// rusEFI Wideband: Little Endian
frame.data[2] = lambda_val & 0xFF;
frame.data[3] = (lambda_val >> 8) & 0xFF;

// EPIC variable_set: Big Endian
writeInt32BigEndian(varHash, &txMsg.data[0]);
```

**Issue:** Kedua protocol di bus yang sama tapi byte order berbeda. Tidak conflict (CAN ID berbeda) tapi bisa membingungkan debugging.

### 17.9. VSS Overflow Reset Paksa

**Lokasi:** `mega_epic_canbus.ino`

```cpp
else if (timeDelta >= 1000) {
    lastCalcTime = now;
    for (uint8_t i = 0; i < 4; ++i) {
        vssChannels[i].lastCount = vssChannels[i].edgeCount;
        vssChannels[i].pulsesPerSecond = 0.0f;
    }
}
```

**Issue:** Reset paksa semua VSS nilai ke 0 saat millis() overflow. Lompatan nilai dari kecepatan tinggi ke 0 bisa memicu false positive pada traction control.

### 17.10. Databox Buffer Overflow Risk

**Lokasi:** `DataboxManager.cpp:pumpData()`

```cpp
if (lineLength_ < (DATABOX_LINE_BUFFER_SIZE - 1)) {
    lineBuffer_[lineLength_++] = ch;
} else {
    lineLength_ = 0;
}
```

**Issue:** Jika frame Databox > 96 byte, buffer di-reset tanpa processing. Frame fix 32 char + CRLF, aman dalam kondisi normal, tapi noise serial bisa menyebabkan overflow.

**Rekomendasi:** Tambah buffer jadi 128 byte untuk safety margin.

---

## 18. ROADMAP & STATUS

### Status Saat Ini: Phase 1 + Merge GearIndicatorCan ✅

| Modul | Status | Detail |
|-------|--------|--------|
| CAN Infrastructure | ✅ Selesai | MCP2515 init, 500 kbps, filter hardware, retry + clock fallback |
| Analog Input | ✅ Selesai | 16 channel, smart TX |
| Digital Input | ✅ Selesai | 11-bit bitfield (D27-D37), smart TX |
| VSS Wheel Speed | ✅ Selesai | 4 channel interrupt-driven, smart TX |
| GPS Module | ✅ Selesai | NMEA parsing, smart TX, packing |
| **Gear Detection** | ✅ **Selesai** | **5 switch input (D22-D26), validasi popcount, smart TX** |
| **AFR Databox** | ✅ **Selesai** | **Serial3 (D14/D15) 57600 baud, state machine reconnect, hex parsing** |
| **Wideband CAN 0x190/0x191** | ✅ **Selesai** | **rusEFI Native Wideband protocol, Little Endian, 10ms interval** |
| Smart Transmission | ✅ Selesai | 30 channel state machine (+1 gear) |
| Big-endian Utils | ✅ Selesai | int32/float32 read/write |
| Slow GPIO Output | ✅ Selesai | Request/response, digitalWrite |
| **CAN Error Handling** | ✅ **Selesai** | **Clock fallback (16MHz→8MHz), auto re-init, max retry 3x** |
| PWM Output | ⚠️ Partial | Hanya on/off, belum duty cycle |

### Belum Dimulai ❌

| Modul | Prioritas | Catatan |
|-------|-----------|---------|
| Register VAR_HASH_GEAR ke epicEFI | ✅ Selesai | Hash 283558758 sudah didaftarkan |
| True PWM Duty Cycle | Tinggi | Gunakan nilai float dari ECU |
| Interrupt-driven CAN RX | Sedang | Kurangi CPU load, cegah frame loss |
| Error Handling TX/RX | Sedang | TX failure, bus-off recovery |
| Watchdog Timer | Sedang | Deteksi ECU communication loss |
| EEPROM Configuration | Rendah | Simpan ecuCanId, mapping |
| Debouncing Digital Input | Rendah | Optional, bitfield change sudah akurat |
| Testing dengan ECU | Tinggi | Integration test gear + AFR + wideband |

### Roadmap

```
Phase 1 [SELESAI]
  ├── CAN Infrastruktur
  ├── Semua Input (analog, digital 16-bit, VSS, GPS)
  ├── Smart Transmission
  └── Output Dasar (request/response)

Phase 2 [SELESAI SEBAGIAN]
  ├── Slow GPIO Output ✅
  ├── PWM Output (on/off) ⚠️
  ├── └── True PWM Duty Cycle ❌
  ├── Gear Detection ✅ [BARU]
  ├── AFR Databox Serial3 ✅ [BARU]
  ├── rusEFI Wideband CAN 0x190/0x191 ✅ [BARU]
  └── CAN Error Handling ✅ [BARU]

Phase 3 [BELUM]
  ├── EPIC Protocol Parser Lengkap
  ├── Error Handling & Recovery
  ├── Watchdog Timer
  ├── Interrupt-driven CAN RX
  └── Integration Test Gear + AFR + ECU

Phase 4 [BELUM]
  ├── Production Hardening
  ├── Performance Optimization
  ├── EEPROM Configuration
  └── Full Integration Test
```

### Perubahan Pin dari Versi Sebelumnya

| Pin | Sebelum | Sesudah |
|-----|---------|---------|
| D14 | Spare GPIO | **Serial3 TX — Databox AFR** |
| D15 | Spare GPIO | **Serial3 RX — Databox AFR** |
| D22-D26 | Digital input (bit 0-4) | **Gear selector (N,1,2,3,4)** |
| D27-D36 | Digital input (bit 5-14) | 10 button inputs | INPUT_PULLUP |
| **D37** | **Clutch switch (bit 15)** | **Switch ke GND saat ditekan** | INPUT_PULLUP |

---

## 19. REFERENSI

### File Firmware
- `mega_epic_canbus.ino` — Main firmware (940 baris)
- `nmea_parser.cpp` — NMEA parser (469 baris)
- `nmea_parser.h` — NMEA parser header (51 baris)

### Dokumentasi
- `README.md` — Overview proyek
- `PINOUT.md` — Mapping pin lengkap
- `DOKUMENTASI.md` — Dokumen ini

### Memory Bank (.project/)
- `projectbrief.md` — Identitas & goals proyek
- `productContext.md` — Problem & solution
- `activeContext.md` — Status terkini
- `systemPatterns.md` — Arsitektur & pola desain
- `techContext.md` — Hardware/software constraints
- `progress.md` — Tracking implementasi detail
- `epic_can_bus_spec.txt` — Spesifikasi protokol EPIC

### Libraries
- [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) — MCP2515 CAN library
- Arduino SPI (built-in)

### Hardware References
- ATmega2560 Datasheet (Microchip)
- MCP2515 Datasheet (Microchip)
- TJA1050 Datasheet (NXP)
- u-blox 7 / NEO-6M GPS Receiver Description

### EpicEFI References
- epicEFI firmware: `firmware/controllers/can/epic_can.h`
- Variable lookup: `firmware/console/binary/value_lookup_generated.cpp`
- Function registry: `kEpicFunctions[]`

---

*Dokumentasi ini dibuat berdasarkan analisis menyeluruh terhadap kode sumber dan file proyek MEGA_EPIC_CANBUS. Untuk informasi terkini, lihat file di direktori `.project/`.*
