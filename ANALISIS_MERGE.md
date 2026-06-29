# Analisis Integrasi: GearIndicatorCan → MEGA_EPIC_CANBUS

## Ringkasan Fitur yang Akan Di-Merge

| # | Fitur | Status | File Target | Tingkat Kesulitan |
|---|-------|--------|-------------|------------------|
| 1 | **Gear Position Detection** — 5 switch input (N,1,2,3,4) via GPIO | Baru | `mega_epic_canbus.ino` | Sedang |
| 2 | **AFR Databox Protocol via Serial3 (D14/D15)** — Baca AFR wideband 57600 baud | Baru | `DataboxManager.h/.cpp` | Tinggi |
| 3 | **rusEFI Native Wideband CAN (0x190/0x191)** — Kirim AFR ke ECU | Baru | `mega_epic_canbus.ino` | Rendah |
| 4 | ~~Haltech ECU Broadcast Parsing~~ | **SKIP** | — | — |
| 5 | **CAN Error Handling** — Retry + clock fallback + rate-limited log | Upgrade | `mega_epic_canbus.ino` | Rendah |

---

## Analisis Detail per Fitur

### 1. GEAR POSITION DETECTION

**Original (ESP32-C3):**
- 5 GPIO: `GEAR_N_PIN=0, GEAR_1_PIN=1, GEAR_2_PIN=3, GEAR_3_PIN=10, GEAR_4_PIN=21`
- INPUT_PULLUP, active LOW
- Validasi: `__builtin_popcount(activeMask) == 1` — hanya 1 gear aktif
- Kirim CAN ID `0x200` byte[4] = gear value (0-4, 0xFF=invalid)

**Porting ke Mega2560 — Alokasi Pin:**

| Gear | Pin ESP32 | Pin Mega2560 | Status | Alasan |
|------|-----------|-------------|--------|--------|
| Neutral | GPIO 0 | **D38** | ✅ Spare GPIO | Tidak conflict |
| Gear 1 | GPIO 1 | **D10** | ✅ Spare PWM GPIO | Tidak conflict |
| Gear 2 | GPIO 3 | **D0** | ⚠️ Serial0 RX | Kehilangan Serial RX (TX tetap bisa) |
| Gear 3 | GPIO 10 | **D1** | ⚠️ Serial0 TX | Kehilangan Serial TX |
| Gear 4 | GPIO 21 | **D2** | ⚠️ CAN INT pin | Kehilangan interrupt-driven CAN (masih polling) |

**Total pin baru:** 5 pin (D38, D10, D0, D1, D2)
**Yang dikorbankan:** Serial0 RX/TX untuk interactive debug, D2 untuk future CAN interrupt

**Perubahan kode:**
- `__builtin_popcount` → `__builtin_popcount` **tidak ada di AVR GCC**. Ganti dengan bit count manual atau lookup table 5-bit.
- CAN ID `0x200` — perlu disesuaikan ke EPIC variable_set (hash baru) ATAU kirim sebagai raw CAN ID 0x200.
  
  **Rekomendasi:** Buat variable hash EPIC baru `MEGA_EPIC_1_GEAR` → kirim via `sendVariableSetFrame()`. Tambah di `variables.json`.

### 2. AFR DATABOX VIA SERIAL3

**Original (ESP32-C3):**
- `HardwareSerial DataboxSerial(1)` — UART1 pada pin RX=20, TX=2
- 57600 baud, 8N1
- State machine: STOP → START_MAIN → START_STREAM → CONNECTED
- Frame hex 32 karakter prefix `"3628"`:
  ```
  [0-3]:  Signature "3628"
  [4-7]:  AFR (hex) /100
  [8-11]: RPM (hex) → uint16
  [12-13]: TPS (hex) → uint8
  [14-16]: EOT (hex) /10 → float °C
  [17-19]: Vbatt (hex) /100 → float
  [20-22]: Ur (hex) /1000 → float (untuk temp probe)
  [23-26]: Duty (hex) /100 → float
  [27-28]: Status (2 char)
  [29-31]: UrCal (hex) /1000 → float
  ```
- Probe temperature lookup table (600-1000°C dari resistansi)
- AFR valid range: 7.0 – 80.0

**Porting ke Mega2560 — Opsi 1 (Serial3):**

| Parameter | ESP32-C3 | Mega2560 |
|-----------|----------|----------|
| UART | `HardwareSerial(1)` | **Serial3** (fixed) |
| RX Pin | GPIO 20 | **D15** (RX3) |
| TX Pin | GPIO 2 | **D14** (TX3) |
| Baud | 57600 | 57600 ✅ |

**Perubahan yang diperlukan:**
- Hapus parameter RX/TX pin dari constructor (Serial3 fixed)
- `begin()` langsung `Serial3.begin(57600)`
- Tidak ada `ESP.restart()` → ganti `while(1)` atau watchdog
- Tidak ada `__builtin_popcount` → ganti manual
- `Serial.printf()` format specifiers → perlu penyesuaian (beberapa format AVR vs ESP32 berbeda)

**Konflik:**
- Serial3 (D14/D15) **tidak conflict** dengan modul manapun ✅
- Tidak perlu wiring ulang yang sudah ada ✅

### 3. rusEFI WIDEBAND CAN (0x190/0x191)

**Original (ESP32-C3):**

Frame 0x190 (Little Endian):
```
[0]: Version (0xA0)
[1]: Valid Flag (0x00=invalid, 0x01=valid)
[2-3]: Lambda (uint16 × 10000) — LSB first
[4-5]: Temperature °C (uint16) — LSB first
[6-7]: Pad (0x00)
```

Frame 0x191 (Diagnostic):
```
[0-1]: ESR (dummy 0)
[2-3]: NernstDC (dummy 0)
[4]:   PumpDuty (dummy 0)
[5]:   Status (0x01=OK)
[6]:   HeaterDuty (0-100%)
[7]:   Reserved
```

Dikirim setiap 10ms.

**Yang perlu diubah untuk Mega2560:**
- `RUSEFI_WIDEBAND_CAN_ID = 0x190` → sudah tidak konflik dengan EPIC range (0x700-0x780)
- TAPI CAN hardware filter saat ini hanya menerima `0x721`. **Perlu perluas filter** atau pakai mask yang lebih longgar.
- Frame menggunakan **Little Endian** — berbeda dengan EPIC yang Big Endian. PENTING untuk konsisten.

### 5. CAN ERROR HANDLING

**Fitur baru dari GearIndicatorCan:**

| Fitur | Deskripsi |
|-------|-----------|
| Clock fallback | Coba 8MHz dulu, gagal → 16MHz |
| Auto re-init | Re-init setiap 3 detik jika CAN down |
| Max retry | 3 siklus gagal → restart (ganti `while(1)` untuk Mega) |
| Rate-limited log | Error hanya dicetak setiap 2 detik |
| MCP probe | Baca register CANSTAT/CANCTRL/EFLG via SPI langsung |

**Implementasi di Mega2560:**
- Tidak ada `ESP.restart()` → `while(1)` dengan Serial message atau watchdog timer
- `spiReadRegister()` — bisa langsung di-copy dari GearIndicatorCan (fungsi SPI sama)

---

## Perubahan pada CAN Hardware Filter

**Kondisi saat ini:** MCP2515 filter hanya menerima CAN ID `0x721` (EPIC variable response).

**Setelah integrasi:** Perlu menerima juga:
- `0x190` — rusEFI Wideband (jika ada ECU lain yang broadcast)
- `0x721` — EPIC variable response (tetap)

**Solusi:** Gunakan mask yang lebih longgar atau set filter ke mode promiscuous.

**Rekomendasi:** Set MASK ke `0x000` (accept all) — paling sederhana.
```cpp
CAN.setFilterMask(MASK0, false, 0x000);  // Accept all
```

Atau alternatif: set 1 filter untuk 0x721, sisanya untuk 0x190.

---

## Estimasi Perubahan Kode

| File | Baris Baru | Perubahan |
|------|-----------|-----------|
| `DataboxManager.h` | ~90 | Port dari ESP32, sesuaikan Serial3 |
| `DataboxManager.cpp` | ~250 | Port dari ESP32, hapus ESP-specific |
| `mega_epic_canbus.ino` | +~150 | Gear detection + Wideband TX + error handling |
| **Total** | **~490 baris baru** | |

## SRAM Impact

| Komponen Baru | Ukuran |
|--------------|--------|
| DataboxManager state & buffer | ~120 bytes |
| Gear state | ~10 bytes |
| Wideband CAN frame state | ~10 bytes |
| **Total tambahan** | **~140 bytes** (dari ~800 used → ~940, masih aman di 8KB) |

---

## Rekomendasi Eksekusi

1. **Buat `DataboxManager.h`** — port class dengan Serial3
2. **Buat `DataboxManager.cpp`** — port semua logic UART + hex parsing
3. **Modifikasi `mega_epic_canbus.ino`:** 
   - Tambah include DataboxManager
   - Tambah gear pin config dan gear detection function
   - Tambah sendWidebandFrame()
   - Tambah CAN init dengan retry + clock fallback
   - Integrasi DataboxManager.update() di loop
   - Perluas CAN hardware filter
4. **Update `variables.json`** — tambah hash gear variable
5. **Update `DOKUMENTASI.md`** — dokumentasi fitur baru
6. **Update `PINOUT.md`** — update pin allocation

Lanjut implementasi?
