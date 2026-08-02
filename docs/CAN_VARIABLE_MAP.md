# EPIC CAN Variable Map — MEGA_EPIC_1

This document lists the variables that are valid for EPIC CAN bus get/set operations. The source is `variables.json` parsed from the epicEFI build log.

> Rule of thumb: **use the packed/broadcast variables**, not individual bit variables.

## Valid CAN variables (≈ 30)

| Variable name | Hash | Direction | Notes |
|---------------|------|-----------|-------|
| `MEGA_EPIC_1_A0` | `595545759` | get / set | Analog input A0 |
| `MEGA_EPIC_1_A1` | `595545760` | get / set | Analog input A1 |
| `MEGA_EPIC_1_A2` | `595545761` | get / set | Analog input A2 |
| `MEGA_EPIC_1_A3` | `595545762` | get / set | Analog input A3 |
| `MEGA_EPIC_1_A4` | `595545763` | get / set | Analog input A4 |
| `MEGA_EPIC_1_A5` | `595545764` | get / set | Analog input A5 |
| `MEGA_EPIC_1_A6` | `595545765` | get / set | Analog input A6 |
| `MEGA_EPIC_1_A7` | `595545766` | get / set | Analog input A7 |
| `MEGA_EPIC_1_A8` | `595545767` | get / set | Analog input A8 |
| `MEGA_EPIC_1_A9` | `595545768` | get / set | Analog input A9 |
| `MEGA_EPIC_1_A10` | `-1821826352` | get / set | Analog input A10 |
| `MEGA_EPIC_1_A11` | `-1821826351` | get / set | Analog input A11 |
| `MEGA_EPIC_1_A12` | `-1821826350` | get / set | Analog input A12 |
| `MEGA_EPIC_1_A13` | `-1821826349` | get / set | Analog input A13 |
| `MEGA_EPIC_1_A14` | `-1821826348` | get / set | Analog input A14 |
| `MEGA_EPIC_1_A15` | `-1821826347` | get / set | Analog input A15 |
| `MEGA_EPIC_1_D22_D37` | `2138825443` | get / set | Packed 16-bit digital input |
| `MEGA_EPIC_1_PWM_T1_D11` | `1015047883` | get only | PWM output D11 status |
| `MEGA_EPIC_1_PWM_T1_D12` | `1015047884` | get only | PWM output D12 status |
| `MEGA_EPIC_1_PWM_T3_D3` | `811734046` | get only | PWM output D3 status |
| `MEGA_EPIC_1_PWM_T3_D5` | `811734048` | get only | PWM output D5 status |
| `MEGA_EPIC_1_PWM_T4_D6` | `811769986` | get only | PWM output D6 status |
| `MEGA_EPIC_1_PWM_T4_D7` | `811769987` | get only | PWM output D7 status |
| `MEGA_EPIC_1_PWM_T4_D8` | `811769988` | get only | PWM output D8 status |
| `MEGA_EPIC_1_PWM_T5_D44` | `1019791669` | get only | PWM output D44 status |
| `MEGA_EPIC_1_PWM_T5_D45` | `1019791670` | get only | PWM output D45 status |
| `MEGA_EPIC_1_PWM_T5_D46` | `1019791671` | get only | PWM output D46 status |
| `MEGA_EPIC_1_OUT_SLOW` | `1430780106` | get / broadcast | Packed slow GPIO + PWM bitfield |
| `mega_epic_slow_changed` | `-861443668` | get only | Flag indicating slow outputs changed |
| `disable_mega_epic_slow_out` | `1435240493` | config | Disable slow output broadcast |

## Invalid / do not use for CAN get/set (≈ 24)

These names exist in firmware / TunerStudio but are skipped in the EPIC CAN lookup table. Use the packed variable above instead.

- `MEGA_EPIC_1_D22` … `MEGA_EPIC_1_D37` — individual digital input bits. Pack into `MEGA_EPIC_1_D22_D37`.
- `MEGA_EPIC_1_SLOW_D39` … `MEGA_EPIC_1_SLOW_D49` — individual slow outputs. Unpack from `MEGA_EPIC_1_OUT_SLOW`.

## MEGA_EPIC_1_OUT_SLOW bitfield layout

| Bits | Meaning | Pins (Mega2560) |
|------|---------|-----------------|
| 0–7  | Slow GPIO outputs | D39, D40, D41, D42, D43, D47, D48, D49 |
| 8–17 | PWM outputs | D3, D5, D6, D7, D8, D11, D12, D44, D45, D46 |

The ECU sends this value as a float; cast / round to `uint32_t` and unpack.

## MEGA_EPIC_1_D22_D37 bitfield layout

| Bit | Pin | Logic |
|-----|-----|-------|
| 0  | D22 | LOW = bit set |
| 1  | D23 | LOW = bit set |
| …  | …   | … |
| 15 | D37 | LOW = bit set |

`INPUT_PULLUP` is enabled on these pins, so a grounded pin reports `1`.

## Quick reference for this project

| Signal | Variable | Hash | Pin | Notes |
|--------|----------|------|-----|-------|
| QuickShifter / load cell ADC | `MEGA_EPIC_1_A0` | `595545759` | A0 | raw 0–1023 ADC counts |
| Clutch switch | `MEGA_EPIC_1_D22_D37` bit 0 | `2138825443` | D4 | `1` when grounded (LOW) |

See `docs/TUNERSTUDIO_SETUP.md` for wiring the variables into rusEFI.
