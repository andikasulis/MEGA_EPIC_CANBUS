# TunerStudio Setup for NANO_EPIC_CANBUS

This guide explains how to configure rusEFI TunerStudio so that the quickshifter ADC and clutch switch sent by the Arduino Nano are used by the ECU.

## What the Nano sends

| Signal | EPIC variable | Hash | Data type | Value |
|--------|---------------|------|-----------|-------|
| QuickShifter / Load cell | `MEGA_EPIC_1_A0` | `595545759` | float32 → U16 | raw ADC counts `0..1023` |
| Clutch switch | `MEGA_EPIC_1_D22_D37` bit 0 | `2138825443` | float32 → U16 | packed digital word; bit 0 = clutch |

CAN frame format for both:
- CAN ID: `0x780 + ECU_CAN_ID` (default `0x781` because `ECU_CAN_ID = 1`)
- DLC: 8
- Bytes 0-3: variable hash, big-endian
- Bytes 4-7: value as float32, big-endian

## 1. Load cell / quickshifter ADC

In TunerStudio:
1. Open **Controller → Load Cell Settings**.
2. Set **Load cell 1 ADC channel** to `MEGA_EPIC_CANBUS_ADC_0`.
3. Set **Load cell 1 minimum value** and **Load cell 1 maximum value** to the voltage range you expect.

Why `MEGA_EPIC_CANBUS_ADC_0`?  
The ECU maps `MEGA_EPIC_CANBUS_ADC_0` to `MEGA_EPIC_1_A0` internally. The Nano sends raw ADC counts (`0..1023`) for `MEGA_EPIC_1_A0`; the ECU stores it as a U16 in `outputChannels`.

### Expected raw ADC range
- Nano ATmega328P ADC is 10-bit, so 0 = 0 V, 1023 ≈ 5 V.
- The load-cell amplifier output should be wired to Nano pin **A0**.

## 2. Clutch Down switch

If your TunerStudio build lists MEGA_EPIC pins in the `clutchDownPin` dropdown, use the direct mapping below. If not, fall back to the Lua option.

### Option A — Direct pin mapping (recommended if available)

1. Open **Controller → Launch Control / Flat Shift** (or wherever `clutchDownPin` is located).
2. Set **Clutch Down** to `MEGA_EPIC_D22`.
3. Set **Clutch Down mode** to `INVERTED PULLUP`.

Why `INVERTED PULLUP`?  
The Nano enables `INPUT_PULLUP` on D4 and reads LOW when the clutch switch grounds the pin. rusEFI sees `MEGA_EPIC_D22` = 1 when grounded. With `INVERTED PULLUP`, the ECU interprets that active-low signal as **clutch down = true**.

### Option B — Lua script

If `MEGA_EPIC_D22` is not in the dropdown, use Lua:

```lua
local digitalWord = getOutput("MEGA_EPIC_1_D22_D37") or 0
local clutchDown = (digitalWord & 1) ~= 0
setClutchDownState(clutchDown)
```

> `setClutchDownState` availability depends on your rusEFI firmware build.

### Wiring on Nano

| Nano pin | Function |
|----------|----------|
| D4 | Clutch switch, `INPUT_PULLUP`, LOW = clutch pressed |

In TunerStudio the physical pin name is irrelevant; only the EPIC variable and bit matter.

## 3. Verifying CAN reception

The ECU exposes debug fields for the last received CAN frame:

| Field | Offset | Meaning |
|-------|--------|---------|
| `lastCanFrameId` | 1624 | CAN ID of last received frame |
| `lastCanFrameData0..7` | 1626-1633 | First 8 data bytes |

In TunerStudio add these fields to a gauge or log. When the Nano is online:
- `lastCanFrameId` should show `0x781` (or your configured ID).
- `lastCanFrameData0..3` should contain the variable hash.
- `lastCanFrameData4..7` should contain the float value.

## 4. Troubleshooting

| Symptom | Check |
|---------|-------|
| `MEGA_EPIC_1_A0` stays 0 | Verify CAN frame leaves Nano (`TX_OK` rises, `ERR_ADC=0`). Verify load-cell amp output on A0. |
| `MEGA_EPIC_1_D22_D37` stays 0 | Press clutch and watch `DIGITAL` in serial log; it should change from `0` to `1`. |
| `lastCanFrameId` does not change | ECU is not receiving frames. Check CAN bus wiring, termination resistors (120 Ω both ends), and MCP2515 crystal setting. |
| `TX_ERR` rises | MCP2515 cannot send; check CS pin, SPI wiring, and crystal frequency (`MCP_8MHZ` vs `MCP_16MHZ`). |

