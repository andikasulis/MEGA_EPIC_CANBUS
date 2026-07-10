## MEGA_EPIC_CANBUS

Arduino Mega2560 firmware that expands epicEFI ECU I/O over CAN bus using an MCP2515-based shield. Provides 16 analog inputs, 5 gear selector inputs, 11 digital inputs, 4 VSS wheel speed sensors, GPS input, AFR wideband input, and 8 slow GPIO + 10 PWM outputs, all via the EPIC_CAN_BUS protocol at 500 kbps.

![Example MCP2515 Hookup](images/conn1.png)

*Example hookup of the MCP2515 CAN shield to the Arduino Mega2560. Ensure your wiring matches this diagram, especially for SPI lines Interrupt (D2) and CS (D9).*

### Features
- **16 analog inputs**: A0–A15 (0–5V)
- **5 gear selector inputs**: D22–D26 (Neutral, 1, 2, 3, 4) with validation
- **11 digital inputs**: D27–D36 (button bitfield) + D37 (clutch switch)
- **4 VSS wheel speed inputs**: D18–D21 (interrupt-driven, falling edge)
- **GPS input over Serial2**: NMEA‑0183 (`GPRMC`/`GPGGA`) parsed and sent to ECU over CAN
- **GPS debug logging**: Serial console output (1s interval) showing fix, coords, speed, etc.
- **AFR wideband input over Serial3**: Databox protocol (BRT) at 57600 baud, D14/D15
- **rusEFI Native Wideband CAN**: Sends AFR/lambda/temperature on CAN ID 0x190/0x191
- **8 slow GPIO outputs**: D39–D43, D47–D49 (from ECU variable_request/response)
- **10 PWM outputs**: D3, D5–D8, D11, D12, D44–D46 (from ECU variable_request/response)
- **CAN error handling**: Auto-reinit with clock fallback (16MHz → 8MHz)
- **Smart transmission**: On-change + heartbeat for all inputs (25ms fast, 500ms slow)
- EPIC protocol operations: variable request/response, variable set, function call

### Status
- Current (Phase 1 + GearIndicatorCan merge):
  - CAN TX/RX with error recovery (retry + clock fallback)
  - All inputs sampled and transmitted using smart on-change + heartbeat strategy
  - Gear detection with validation (only 1 gear active at a time)
  - AFR wideband via Databox protocol (Serial3) + rusEFI Native Wideband CAN
  - GPS (time, date, position, speed, course, altitude, quality, satellites)
  - Digital output request/response from ECU (slow GPIO + PWM on/off)
- Missing: True PWM duty cycle, interrupt-driven CAN RX, EEPROM configuration

### Hardware
- Arduino Mega2560
- MCP2515‑based CAN shield (e.g. generic MCP_CAN shield)
- Default GPS module: GT‑U7 (u‑blox 7) style module  
  (e.g. [GT‑U7 (u‑blox7) module](https://www.amazon.com/dp/B08MZ2CBP7?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_13))
- Optional: AFR wideband controller with Databox/UART output (57600 baud)

#### Wiring Notes (Important)
- **SPI / MCP2515 (via ICSP Header):**
  - Use the **6‑pin ICSP header in the center of the Mega2560** for SPI (MISO/MOSI/SCK).  
    Many MCP2515 shields have a matching 2×3 header that should plug directly into the ICSP header.
  - MCP2515 wiring: `CS` → **D9**, `INT` → **D2**, `SO` → ICSP MISO, `SI` → ICSP MOSI, `SCK` → ICSP SCK
  - CAN_H/CAN_L twisted pair with proper 120Ω termination at both ends of the bus.
- **GPS UART (Serial2):**
  - D16 (TX2), D17 (RX2) @ 115200 baud
- **AFR Databox:**
  - Connect AFR controller TX to D15 (Serial3 RX) and RX to D14 (Serial3 TX).
  - Use 57600 baud, 8N1.

### Pin Map Summary
- **Analog inputs**: A0–A15
- **Gear inputs**: D22–D26 (Neutral, 1, 2, 3, 4)
- **Digital button inputs**: D27–D36 (10-bit packed) + D37 (clutch switch)
- **VSS inputs** (wheel speed):
  - D18: Front Left (INT3)
  - D19: Front Right (INT2)
  - D20: Rear Left (INT1, I2C SDA disabled)
  - D21: Rear Right (INT0, I2C SCL disabled)
- **GPS UART**: D16 (TX2), D17 (RX2) @ 115200 baud
- **AFR Databox UART**: D14 (TX3), D15 (RX3) @ 57600 baud
- **MCP2515 CAN SPI**: D9 (CS), D2 (INT), ICSP header (MISO/MOSI/SCK)
- **PWM outputs**: D3, D5, D6, D7, D8, D11, D12, D44, D45, D46 (D9 used by CS)
- **Digital low-speed outputs**: D39–D43, D47–D49

### Protocol (EPIC_CAN_BUS)
- Base IDs (11-bit standard):
  - `0x700 + ecuCanId`: Variable request (DLC=4, int32 hash)
  - `0x720 + ecuCanId`: Variable response (hash + float32 value)
  - `0x740 + ecuCanId`: Function request (uint16 id, float32 arg1, optional int16 arg2)
  - `0x760 + ecuCanId`: Function response (uint16 id, return float32)
  - `0x780 + ecuCanId`: Variable set (hash + float32 value)
- Byte order: big-endian for all multi-byte fields

See `.project/epic_can_bus_spec.txt` for full details.

### Getting Started
1. Install Arduino IDE (1.8.x or 2.x)
2. Libraries:
   - `arduino-mcp2515` (autowp MCP2515 CAN interface library)
   - `SPI` (Arduino core)
3. Open `mega_epic_canbus.ino`
4. Board: Arduino Mega or Mega 2560 (ATmega2560)
5. Port: your USB serial port
6. Upload and open Serial Monitor at 115200 baud

### Serial Monitor Output
Open Serial Monitor at **115200 baud** to see debug output:

```
MEGA_EPIC_CANBUS booting...
[CAN] Init with MCP_16MHZ...
[CAN] Ready (16MHz)
[GPS] Waiting for data on Serial2...
[GPS] First valid data received!
[GPS] fix=Y q=1 sats=8 time=23:09:52 date=03/07/2026 lat=-6.123456 lon=106.123456 alt=12.3 spd=45.6 crs=180.0 hdop=1.20
[CAN] Probe CANSTAT=0x00 CANCTRL=0x80 EFLG=0x00
MEGA_EPIC_CANBUS ready!
```

GPS log format (1s interval):
- `fix` = Y/N (GPS fix status)
- `q` = quality (0=none, 1=GPS, 2=DGPS)
- `sats` = number of satellites
- `time` = UTC (HH:MM:SS)
- `date` = (DD/MM/YYYY)
- `lat/lon` = decimal degrees (6 decimal)
- `alt` = altitude meters
- `spd` = speed km/h
- `crs` = course/heading degrees
- `hdop` = horizontal dilution of precision

### Configuration
- `ecuCanId` (0–15): per-device address used to derive CAN IDs (e.g., `0x700 + ecuCanId`). Define this in code and in docs. If not chosen, default to `1` in early testing.
- `CS` pin: **D9** (matches shield default and reserved in firmware)
- GPS:
  - Default baud rate: **115200** (tuned for GT‑U7 / u‑blox 7‑class modules that support 115200 and 20 Hz updates).
  - Default NMEA update rate: **20 Hz** (module must support this; slower 9600/1–5 Hz receivers can be used by lowering `GPS_BAUD_RATE` / `GPS_UPDATE_RATE_HZ` in code).

### Repository Structure
- `mega_epic_canbus.ino` — main sketch (setup, basic CAN RX demo)
- `variables.json` — pre-generated variable names and hashes for I/O mapping
- `nmea_parser.h`, `nmea_parser.cpp` — NMEA GPS message parsing (for GPS input)
- `arduino-mcp2515-master/` — provided CAN bus library (autowp MCP2515, unzip and install as Arduino library)
- `.project/` — Memory Bank (project intent, architecture, specs)
  - `projectbrief.md` — identity, goals, architecture
  - `productContext.md` — problem/solution overview
  - `activeContext.md` — current work focus and next steps
  - `systemPatterns.md` — architecture, patterns, module plan
  - `techContext.md` — hardware/software constraints
  - `epic_can_bus_spec.txt` — protocol summary

### Roadmap
- Define and store `ecuCanId`
- Implement EPIC frame parsing and TX helpers
- Map variables from `variables.json` to pins (analogs, digital inputs)
- Implement analog input sampling and variable_set TX with throttling
- Implement digital input bitfield TX and output application
- Add PWM output control
- Enable interrupt-driven CAN RX
- Add watchdog and error handling

### Usage (planned behavior)
- Mega periodically samples A0–A15 and sends values as `variable_set` frames
- Digital inputs (D20–D34) packed into a bitfield and sent on change/interval
- ECU responses drive digital outputs (D35–D49) and PWM pins

### Performance Targets
- 500 kbps CAN
- 400–700 frames/sec practical throughput
- <10 ms latency for critical I/O updates

### Contributing
Issues and PRs welcome. Keep changes modular and avoid dynamic allocation on AVR. Reference the Memory Bank docs in `.project/` for architecture and constraints.

### License
TBD. If you intend to contribute a license, add a `LICENSE` file and update this section.


