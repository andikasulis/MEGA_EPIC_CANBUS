/*
  MEGA_EPIC_CANBUS - Arduino Mega2560 CAN I/O Expander Firmware
  --------------------------------------------------------------
  Expands epicEFI ECU input/output via CAN bus using the EPIC_CAN_BUS protocol.
  Provides analog inputs, PWM outputs, digital inputs/outputs for advanced ECU interfacing.

  Merged features from GearIndicatorCan:
  - Gear position detection (D22-D26: N, 1, 2, 3, 4)
  - AFR Databox wideband via Serial3 (D14/D15) at 57600 baud
  - rusEFI Native Wideband CAN protocol (0x190/0x191)
  - CAN error handling with clock fallback + retry

  Author: Gennady Gurov
  Made for: epicEFI project
*/

#include <SPI.h>
#include <stdint.h>
#include <math.h>
#include <mcp2515.h>
#include <string.h>
#include "nmea_parser.h"
#include "DataboxManager.h"

#define BOARD_CAN_CLOCK MCP_16MHZ
#define SPI_CS_PIN  9

MCP2515 CAN(SPI_CS_PIN);

// ─── CAN Error Handling ──────────────────────────────────────────
#define CAN_RETRY_CLOCKS      2       // 8MHz, 16MHz
#define CAN_REINIT_INTERVAL_MS 3000
#define CAN_REINIT_MAX_CYCLES  3
#define ERROR_LOG_INTERVAL_MS 2000
bool canReady = false;

// ─── EPIC CAN Configuration ──────────────────────────────────────
#define ECU_CAN_ID  1
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)

// ─── rusEFI Wideband CAN ─────────────────────────────────────────
#define RUSEFI_WIDEBAND_CAN_ID   0x190
#define RUSEFI_WIDEBAND_VERSION  0xA0
#define DATABOX_SEND_INTERVAL_MS 10
#define WIDEBAND_SEND_INTERVAL_MS 10
#define STOICH_GASOLINE          14.7f

// ─── Timing ──────────────────────────────────────────────────────
#define SLOW_OUT_REQUEST_INTERVAL_MS 25
#define TX_INTERVAL_FAST_MS    25
#define TX_INTERVAL_SLOW_MS    500
#define TX_READ_INTERVAL_MS    10
#define TX_ANALOG_THRESHOLD    2.0f
#define TX_VSS_THRESHOLD       0.1f

// ─── Variable Hashes ─────────────────────────────────────────────
const int32_t VAR_HASH_ANALOG[16] = {
    595545759, 595545760, 595545761, 595545762,
    595545763, 595545764, 595545765, 595545766,
    595545767, 595545768, -1821826352, -1821826351,
   -1821826350, -1821826349, -1821826348, -1821826347
};

// Menggunakan hash existing MEGA_EPIC_1_D20_D34 (sudah di epicEFI)
// Bit 0-1: D20-D21 (VSS, tidak dipakai digital)
// Bit 2-4: D22-D26 (gear, diisi 0)
// Bit 5-15: D27-D37 (11-bit digital inputs)
const int32_t VAR_HASH_DIGITAL_INPUT = 2136453598;

const int32_t VAR_HASH_OUT_SLOW = 1430780106;

// ─── Gear Position Detection ─────────────────────────────────────
// D22-D26 repurposed from digital input to gear selector
#define GEAR_N_PIN  22
#define GEAR_1_PIN  23
#define GEAR_2_PIN  24
#define GEAR_3_PIN  25
#define GEAR_4_PIN  26
#define GEAR_PIN_COUNT 5
const uint8_t GEAR_PINS[5] = {22, 23, 24, 25, 26};

// Hash resmi dari epicEFI: detectedGear = 283558758
const int32_t VAR_HASH_GEAR = 283558758;

enum GearValue : uint8_t {
  kNeutral = 0,
  kGear1 = 1,
  kGear2 = 2,
  kGear3 = 3,
  kGear4 = 4,
  kInvalid = 0xFF
};

struct GearState {
  GearValue gear;
  uint8_t activeMask;
};

// ─── GPS ─────────────────────────────────────────────────────────
const int32_t VAR_HASH_GPS_HMSD_PACKED = 703958849;
const int32_t VAR_HASH_GPS_MYQSAT_PACKED = -1519914092;
const int32_t VAR_HASH_GPS_ACCURACY = -1489698215;
const int32_t VAR_HASH_GPS_ALTITUDE = -2100224086;
const int32_t VAR_HASH_GPS_COURSE = 1842893663;
const int32_t VAR_HASH_GPS_LATITUDE = 1524934922;
const int32_t VAR_HASH_GPS_LONGITUDE = -809214087;
const int32_t VAR_HASH_GPS_SPEED = -1486968225;

// ─── VSS ─────────────────────────────────────────────────────────
#define VSS_FRONT_LEFT_PIN   18
#define VSS_FRONT_RIGHT_PIN  19
#define VSS_REAR_LEFT_PIN    20
#define VSS_REAR_RIGHT_PIN   21
#define VSS_CALC_INTERVAL_MS 25
#define VSS_ENABLE_PULLUP    1

const int32_t VAR_HASH_VSS_FRONT_LEFT  = -1645222329;
const int32_t VAR_HASH_VSS_FRONT_RIGHT = 1549498074;
const int32_t VAR_HASH_VSS_REAR_LEFT   = 768443592;
const int32_t VAR_HASH_VSS_REAR_RIGHT  = -403905157;

struct VSSChannel {
    volatile uint32_t edgeCount;
    uint32_t lastCount;
    unsigned long lastCalcTime;
    float pulsesPerSecond;
};

VSSChannel vssChannels[4] = {{0,0,0,0.0f},{0,0,0,0.0f},{0,0,0,0.0f},{0,0,0,0.0f}};

// ─── Smart Transmission ─────────────────────────────────────────
#define TX_STATE_CHANGED  0
#define TX_STATE_STABLE   1

struct TxChannelState {
    float lastTransmittedValue;
    unsigned long lastTxTime;
    bool hasChanged;
    uint8_t state;
};

TxChannelState analogTxState[16];
TxChannelState digitalTxState;
TxChannelState vssTxState[4];
TxChannelState gpsTxState[8];
TxChannelState gearTxState;

static float currentAnalogValues[16] = {0};
static uint16_t currentDigitalBits = 0;
static float currentVssValues[4] = {0};

// ─── GPS State ───────────────────────────────────────────────────
#define GPS_SERIAL Serial2
#define GPS_BAUD_RATE 115200
#define GPS_UPDATE_RATE_HZ 20
#define GPS_BAUD_RATE_HIGH_SPEED 0

bool gpsEnabled = false;
bool gpsInitialized = false;

static GPSData gpsData = {0};
static GPSData lastTransmittedGpsData = {0};

// ─── Output Pins ────────────────────────────────────────────────
const uint8_t SLOW_GPIO_PINS[8] = {39, 40, 41, 42, 43, 47, 48, 49};
const uint8_t PWM_OUTPUT_PINS[10] = {3, 5, 6, 7, 8, 11, 12, 44, 45, 46};

// ─── Databox AFR ────────────────────────────────────────────────
DataboxManager databox;

// ================================================================
// Internal Helpers
// ================================================================

static inline void writeInt32BigEndian(int32_t value, unsigned char* out)
{
    out[0] = (unsigned char)((value >> 24) & 0xFF);
    out[1] = (unsigned char)((value >> 16) & 0xFF);
    out[2] = (unsigned char)((value >> 8) & 0xFF);
    out[3] = (unsigned char)(value & 0xFF);
}

static inline void writeFloat32BigEndian(float value, unsigned char* out)
{
    union { float f; uint32_t u; } conv;
    conv.f = value;
    out[0] = (unsigned char)((conv.u >> 24) & 0xFF);
    out[1] = (unsigned char)((conv.u >> 16) & 0xFF);
    out[2] = (unsigned char)((conv.u >> 8) & 0xFF);
    out[3] = (unsigned char)(conv.u & 0xFF);
}

static inline int32_t readInt32BigEndian(const unsigned char* in)
{
    int32_t value = 0;
    value |= ((int32_t)in[0] << 24);
    value |= ((int32_t)in[1] << 16);
    value |= ((int32_t)in[2] << 8);
    value |= ((int32_t)in[3]);
    return value;
}

static inline float readFloat32BigEndian(const unsigned char* in)
{
    union { float f; uint32_t u; } conv;
    conv.u =  ((uint32_t)in[0] << 24)
            | ((uint32_t)in[1] << 16)
            | ((uint32_t)in[2] << 8)
            | ((uint32_t)in[3]);
    return conv.f;
}

static struct can_frame txMsg;

static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    txMsg.can_id  = CAN_ID_VARIABLE_SET;
    txMsg.can_dlc = 8;
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    writeFloat32BigEndian(value, &txMsg.data[4]);
    CAN.sendMessage(&txMsg);
}

static inline void sendVariableSetFrameU32(int32_t varHash, uint32_t value)
{
    txMsg.can_id  = CAN_ID_VARIABLE_SET;
    txMsg.can_dlc = 8;
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    writeInt32BigEndian(value, &txMsg.data[4]);
    CAN.sendMessage(&txMsg);
}

static inline void sendVariableRequestFrame(int32_t varHash)
{
    txMsg.can_id = CAN_ID_VAR_REQUEST;
    txMsg.can_dlc = 4;
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    CAN.sendMessage(&txMsg);
}

// ================================================================
// CAN Filter Configuration
// ================================================================

static void configureCANFilters()
{
    // Accept all incoming frames (needed for CAN_ID_VAR_RESPONSE + 0x190)
    CAN.setFilterMask(MCP2515::MASK0, false, 0x000);
    CAN.setFilterMask(MCP2515::MASK1, false, 0x000);
    CAN.setFilter(MCP2515::RXF0, false, CAN_ID_VAR_RESPONSE);
    CAN.setFilter(MCP2515::RXF1, false, CAN_ID_VAR_RESPONSE);
    CAN.setFilter(MCP2515::RXF2, false, CAN_ID_VAR_RESPONSE);
    CAN.setFilter(MCP2515::RXF3, false, CAN_ID_VAR_RESPONSE);
    CAN.setFilter(MCP2515::RXF4, false, CAN_ID_VAR_RESPONSE);
    CAN.setFilter(MCP2515::RXF5, false, CAN_ID_VAR_RESPONSE);
}

// ================================================================
// CAN Initialization with Retry + Clock Fallback
// ================================================================

static void logMcpProbe() {
    // Debug: read MCP2515 registers via direct SPI
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x03); SPI.transfer(0x0E);
    uint8_t canstat = SPI.transfer(0x00);
    digitalWrite(SPI_CS_PIN, HIGH);

    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x03); SPI.transfer(0x0F);
    uint8_t canctrl = SPI.transfer(0x00);
    digitalWrite(SPI_CS_PIN, HIGH);

    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x03); SPI.transfer(0x2D);
    uint8_t eflg = SPI.transfer(0x00);
    digitalWrite(SPI_CS_PIN, HIGH);

    Serial.print(F("[CAN] Probe CANSTAT=0x")); Serial.print(canstat, HEX);
    Serial.print(F(" CANCTRL=0x")); Serial.print(canctrl, HEX);
    Serial.print(F(" EFLG=0x")); Serial.println(eflg, HEX);
}

static bool setupCan(CAN_CLOCK clock) {
    CAN.reset();
    delay(200);
    CAN.setBitrate(CAN_500KBPS, clock);
    if (CAN.getErrorFlags() != 0) return false;
    CAN.setNormalMode();
    return true;
}

static bool initializeCanController() {
    Serial.println(F("[CAN] Init with MCP_16MHZ..."));
    if (setupCan(MCP_16MHZ)) {
        logMcpProbe();
        Serial.println(F("[CAN] Ready (16MHz)"));
        return true;
    }
    Serial.println(F("[CAN] 16MHz failed, trying 8MHz..."));
    if (setupCan(MCP_8MHZ)) {
        logMcpProbe();
        Serial.println(F("[CAN] Ready (8MHz)"));
        return true;
    }
    Serial.println(F("[CAN] Init FAILED"));
    return false;
}

// ================================================================
// VSS Interrupt Service Routines
// ================================================================

void vssFrontLeftISR() {
    if (vssChannels[0].edgeCount < 0xFFFFFFFE) vssChannels[0].edgeCount++;
}
void vssFrontRightISR() {
    if (vssChannels[1].edgeCount < 0xFFFFFFFE) vssChannels[1].edgeCount++;
}
void vssRearLeftISR() {
    if (vssChannels[2].edgeCount < 0xFFFFFFFE) vssChannels[2].edgeCount++;
}
void vssRearRightISR() {
    if (vssChannels[3].edgeCount < 0xFFFFFFFE) vssChannels[3].edgeCount++;
}

// ================================================================
// Gear Detection
// ================================================================

static GearState readGearState() {
    uint8_t mask = 0;
    if (digitalRead(GEAR_N_PIN) == LOW) mask |= 0x01;
    if (digitalRead(GEAR_1_PIN) == LOW) mask |= 0x02;
    if (digitalRead(GEAR_2_PIN) == LOW) mask |= 0x04;
    if (digitalRead(GEAR_3_PIN) == LOW) mask |= 0x08;
    if (digitalRead(GEAR_4_PIN) == LOW) mask |= 0x10;

    // Count active bits (manual popcount for AVR GCC compatibility)
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

// ================================================================
// Smart Transmission Functions
// ================================================================

void readAnalogInputs(float* values) {
    for (uint8_t i = 0; i < 16; ++i) values[i] = (float)analogRead(A0 + i);
}

// Digital inputs now D27-D37 (11 bits), packed into bits 5-15
// Bits 0-1: VSS (D20-D21) — always 0
// Bits 2-4: Gear (D22-D26) — always 0
void readDigitalInputs(uint16_t* bits) {
    *bits = 0;
    for (uint8_t pin = 27; pin <= 37; ++pin) {
        uint8_t bitIndex = (uint8_t)(pin - 27);  // 0-10
        if (digitalRead(pin) == LOW) {
            *bits |= (uint16_t)(1u << (bitIndex + 5));  // Shift ke bit 5-15
        }
    }
}

void readVSSInputs(float* values) {
    for (uint8_t i = 0; i < 4; ++i) values[i] = vssChannels[i].pulsesPerSecond;
}

bool hasAnalogChanged(uint8_t channel, float newValue) {
    float diff = newValue - analogTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff;
    return (diff >= TX_ANALOG_THRESHOLD);
}

bool hasDigitalChanged(uint16_t newBits) {
    return (newBits != (uint16_t)digitalTxState.lastTransmittedValue);
}

bool hasVSSChanged(uint8_t channel, float newValue) {
    float diff = newValue - vssTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff;
    return (diff >= TX_VSS_THRESHOLD);
}

void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs) {
    if (changed) {
        state->hasChanged = true;
        state->state = TX_STATE_CHANGED;
    } else {
        state->hasChanged = false;
        if (state->state == TX_STATE_CHANGED &&
            (nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS) {
            state->state = TX_STATE_STABLE;
        }
    }
}

bool shouldTransmit(TxChannelState* state, unsigned long nowMs) {
    if (state->hasChanged) {
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS);
    } else {
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_SLOW_MS);
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

// ================================================================
// Wideband AFR CAN (rusEFI Native Protocol, Little Endian)
// ================================================================

static float calculateProbeTemp(float ur) {
    static const int temperatures[] = {600, 700, 780, 800, 900, 1000};
    static const int resistances[] = {2200, 550, 300, 260, 160, 80};

    float res = ur * 300.0f;
    int idx = -1;
    for (int i = 0; i < 5; i++) {
        if (res > resistances[i + 1] && res <= resistances[i]) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        if (res > resistances[0]) return (float)temperatures[0];
        if (res <= resistances[5]) return (float)temperatures[5];
        return 0.0f;
    }
    float temp = (res - resistances[idx + 1]) /
                 (resistances[idx] - resistances[idx + 1]) *
                 (temperatures[idx] - temperatures[idx + 1]) +
                 temperatures[idx + 1];
    return temp;
}

static void sendWidebandFrame(float afr, float duty, float ur, bool connected) {
    struct can_frame frame;
    struct can_frame diagFrame;

    frame.can_id = RUSEFI_WIDEBAND_CAN_ID;
    frame.can_dlc = 8;
    diagFrame.can_id = RUSEFI_WIDEBAND_CAN_ID + 1;
    diagFrame.can_dlc = 8;

    if (connected && afr > 0.0f) {
        float clampedAfr = afr;
        if (clampedAfr < 8.0f) clampedAfr = 8.0f;
        if (clampedAfr > 20.0f) clampedAfr = 20.0f;

        float lambda = clampedAfr / STOICH_GASOLINE;
        uint16_t lambda_val = (uint16_t)((lambda * 10000.0f) + 0.5f);
        uint16_t sensor_temp = (uint16_t)(calculateProbeTemp(ur) + 0.5f);

        // 0x190: rusEFI Native Wideband (Little Endian)
        frame.data[0] = RUSEFI_WIDEBAND_VERSION;
        frame.data[1] = 0x01;
        frame.data[2] = lambda_val & 0xFF;
        frame.data[3] = (lambda_val >> 8) & 0xFF;
        frame.data[4] = sensor_temp & 0xFF;
        frame.data[5] = (sensor_temp >> 8) & 0xFF;
        frame.data[6] = 0x00;
        frame.data[7] = 0x00;

        // 0x191: Diagnostic
        diagFrame.data[0] = 0x00;
        diagFrame.data[1] = 0x00;
        diagFrame.data[2] = 0x00;
        diagFrame.data[3] = 0x00;
        diagFrame.data[4] = 0x00;
        diagFrame.data[5] = 0x01;
        diagFrame.data[6] = (uint8_t)(duty + 0.5f);
        diagFrame.data[7] = 0x00;
    } else {
        frame.data[0] = RUSEFI_WIDEBAND_VERSION;
        frame.data[1] = 0x00;
        frame.data[2] = 0x00; frame.data[3] = 0x00;
        frame.data[4] = 0x00; frame.data[5] = 0x00;
        frame.data[6] = 0x00; frame.data[7] = 0x00;

        diagFrame.data[0] = 0x00; diagFrame.data[1] = 0x00;
        diagFrame.data[2] = 0x00; diagFrame.data[3] = 0x00;
        diagFrame.data[4] = 0x00; diagFrame.data[5] = 0x00;
        diagFrame.data[6] = 0x00; diagFrame.data[7] = 0x00;
    }
    CAN.sendMessage(&frame);
    CAN.sendMessage(&diagFrame);
}

// ================================================================
// GPS Functions
// ================================================================

static inline uint32_t packGPSHMSD(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t days) {
    return ((uint32_t)hours) | ((uint32_t)minutes << 8) |
           ((uint32_t)seconds << 16) | ((uint32_t)days << 24);
}

static inline uint32_t packGPSMYQSAT(uint8_t months, uint8_t years, uint8_t quality, uint8_t satellites) {
    return ((uint32_t)months) | ((uint32_t)years << 8) |
           ((uint32_t)quality << 16) | ((uint32_t)satellites << 24);
}

static bool readGPSData() {
    unsigned long nowMs = millis();
    bool dataReceived = false;
    while (GPS_SERIAL.available() > 0) {
        char c = GPS_SERIAL.read();
        if (nmeaParserProcessChar(c, &gpsData)) {
            dataReceived = true;
            if (!gpsEnabled) gpsEnabled = true;
        }
    }
    return dataReceived;
}

static bool hasGPSChanged(uint8_t gpsVarIndex) {
    const float GPS_FLOAT_THRESHOLD = 0.001f;
    switch (gpsVarIndex) {
        case 0: return (packGPSHMSD(gpsData.hours,gpsData.minutes,gpsData.seconds,gpsData.days) !=
                        packGPSHMSD(lastTransmittedGpsData.hours,lastTransmittedGpsData.minutes,
                                    lastTransmittedGpsData.seconds,lastTransmittedGpsData.days));
        case 1: return (packGPSMYQSAT(gpsData.months,gpsData.years,gpsData.quality,gpsData.satellites) !=
                        packGPSMYQSAT(lastTransmittedGpsData.months,lastTransmittedGpsData.years,
                                      lastTransmittedGpsData.quality,lastTransmittedGpsData.satellites));
        case 2: return (fabs(gpsData.accuracy - lastTransmittedGpsData.accuracy) > GPS_FLOAT_THRESHOLD);
        case 3: return (fabs(gpsData.altitude - lastTransmittedGpsData.altitude) > GPS_FLOAT_THRESHOLD);
        case 4: return (fabs(gpsData.course - lastTransmittedGpsData.course) > GPS_FLOAT_THRESHOLD);
        case 5: return (fabs(gpsData.latitude - lastTransmittedGpsData.latitude) > GPS_FLOAT_THRESHOLD);
        case 6: return (fabs(gpsData.longitude - lastTransmittedGpsData.longitude) > GPS_FLOAT_THRESHOLD);
        case 7: return (fabs(gpsData.speed - lastTransmittedGpsData.speed) > GPS_FLOAT_THRESHOLD);
        default: return false;
    }
}

static void transmitGPSIfNeeded() {
    if (!gpsEnabled || !gpsData.dataValid) return;
    unsigned long nowMs = millis();

    // Index 0: packed_hmsd
    bool changed0 = hasGPSChanged(0);
    updateTxState(&gpsTxState[0], changed0, nowMs);
    if (shouldTransmit(&gpsTxState[0], nowMs)) {
        uint32_t value = packGPSHMSD(gpsData.hours,gpsData.minutes,gpsData.seconds,gpsData.days);
        sendVariableSetFrameU32(VAR_HASH_GPS_HMSD_PACKED, value);
        gpsTxState[0].lastTransmittedValue = value;
        gpsTxState[0].lastTxTime = nowMs;
        lastTransmittedGpsData.hours = gpsData.hours;
        lastTransmittedGpsData.minutes = gpsData.minutes;
        lastTransmittedGpsData.seconds = gpsData.seconds;
        lastTransmittedGpsData.days = gpsData.days;
    }

    // Index 1: packed_myqsat
    bool changed1 = hasGPSChanged(1);
    updateTxState(&gpsTxState[1], changed1, nowMs);
    if (shouldTransmit(&gpsTxState[1], nowMs)) {
        uint32_t value = packGPSMYQSAT(gpsData.months,gpsData.years,gpsData.quality,gpsData.satellites);
        sendVariableSetFrameU32(VAR_HASH_GPS_MYQSAT_PACKED, value);
        gpsTxState[1].lastTransmittedValue = value;
        gpsTxState[1].lastTxTime = nowMs;
        lastTransmittedGpsData.months = gpsData.months;
        lastTransmittedGpsData.years = gpsData.years;
        lastTransmittedGpsData.quality = gpsData.quality;
        lastTransmittedGpsData.satellites = gpsData.satellites;
    }

    if (!gpsData.hasFix) return;

    // Indices 2-7: float values (only if has fix)
    for (uint8_t i = 2; i <= 7; ++i) {
        bool changed = hasGPSChanged(i);
        updateTxState(&gpsTxState[i], changed, nowMs);
        if (shouldTransmit(&gpsTxState[i], nowMs)) {
            float val;
            int32_t hash;
            switch (i) {
                case 2: val = gpsData.accuracy; hash = VAR_HASH_GPS_ACCURACY; break;
                case 3: val = gpsData.altitude; hash = VAR_HASH_GPS_ALTITUDE; break;
                case 4: val = gpsData.course;   hash = VAR_HASH_GPS_COURSE; break;
                case 5: val = gpsData.latitude; hash = VAR_HASH_GPS_LATITUDE; break;
                case 6: val = gpsData.longitude; hash = VAR_HASH_GPS_LONGITUDE; break;
                default: val = gpsData.speed;    hash = VAR_HASH_GPS_SPEED; break;
            }
            sendVariableSetFrame(hash, val);
            gpsTxState[i].lastTransmittedValue = val;
            gpsTxState[i].lastTxTime = nowMs;
        }
    }
    lastTransmittedGpsData = gpsData;
}

// ================================================================
// VSS Rate Calculation
// ================================================================

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
        lastCalcTime = now;
        for (uint8_t i = 0; i < 4; ++i) {
            vssChannels[i].lastCount = vssChannels[i].edgeCount;
            vssChannels[i].pulsesPerSecond = 0.0f;
        }
    }
}

// ================================================================
// CAN RX Handler
// ================================================================

static void handleCanFrame(const struct can_frame& rxMsg)
{
    if ((rxMsg.can_id == CAN_ID_VAR_RESPONSE) && (rxMsg.can_dlc == 8))
    {
        int32_t hash = readInt32BigEndian(&rxMsg.data[0]);
        if (hash == VAR_HASH_OUT_SLOW)
        {
            float value = readFloat32BigEndian(&rxMsg.data[4]);
            uint32_t rawBits = (value >= 0.0f) ? (uint32_t)(value + 0.5f) : 0u;

            uint8_t slowBits = (uint8_t)(rawBits & 0xFFu);
            for (uint8_t i = 0; i < 8; ++i) {
                digitalWrite(SLOW_GPIO_PINS[i], (slowBits & (1u << i)) ? HIGH : LOW);
            }

            for (uint8_t i = 0; i < 10; ++i) {
                uint8_t pin = PWM_OUTPUT_PINS[i];
                uint8_t bitIndex = i + 8;
                analogWrite(pin, (rawBits & (1u << bitIndex)) ? 255 : 0);
            }
        }
    }
}

// ================================================================
// SETUP
// ================================================================

void setup()
{
    Serial.begin(115200);
    while(!Serial);
    Serial.println(F("MEGA_EPIC_CANBUS booting..."));

    // ── CAN ──
    if (initializeCanController()) {
        configureCANFilters();
        canReady = true;
    } else {
        Serial.println(F("[CAN] Init failed, will retry in loop"));
    }

    // ── GPS ──
    GPS_SERIAL.begin(GPS_BAUD_RATE);
    delay(200);
    gpsInitialized = true;
    gpsEnabled = false;
    nmeaParserInit();

    // ── Databox AFR ──
    databox.begin();

    // ── Analog Inputs ──
    for (uint8_t i = 0; i < 16; ++i) pinMode(A0 + i, INPUT_PULLUP);

    // ── Gear Inputs (D22-D26) ──
    for (uint8_t i = 0; i < 5; ++i) pinMode(GEAR_PINS[i], INPUT_PULLUP);

    // ── Digital Inputs (D27-D37, 11-bit) ──
    for (uint8_t pin = 27; pin <= 37; ++pin) pinMode(pin, INPUT_PULLUP);

    // ── Outputs ──
    for (uint8_t i = 0; i < 8; ++i) {
        pinMode(SLOW_GPIO_PINS[i], OUTPUT);
        digitalWrite(SLOW_GPIO_PINS[i], LOW);
    }
    for (uint8_t i = 0; i < 10; ++i) {
        pinMode(PWM_OUTPUT_PINS[i], OUTPUT);
        analogWrite(PWM_OUTPUT_PINS[i], 0);
    }

    // ── VSS ──
    TWCR &= ~(1<<TWEN);
    pinMode(VSS_FRONT_LEFT_PIN, INPUT_PULLUP);
    pinMode(VSS_FRONT_RIGHT_PIN, INPUT_PULLUP);
    pinMode(VSS_REAR_LEFT_PIN, INPUT_PULLUP);
    pinMode(VSS_REAR_RIGHT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_LEFT_PIN), vssFrontLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_RIGHT_PIN), vssFrontRightISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_LEFT_PIN), vssRearLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_RIGHT_PIN), vssRearRightISR, FALLING);

    // ── Init Smart TX State ──
    for (uint8_t i = 0; i < 16; ++i) {
        analogTxState[i].lastTransmittedValue = 0.0f;
        analogTxState[i].lastTxTime = 0;
        analogTxState[i].hasChanged = true;
        analogTxState[i].state = TX_STATE_CHANGED;
    }
    digitalTxState.lastTransmittedValue = 0.0f;
    digitalTxState.lastTxTime = 0;
    digitalTxState.hasChanged = true;
    digitalTxState.state = TX_STATE_CHANGED;

    for (uint8_t i = 0; i < 4; ++i) {
        vssTxState[i].lastTransmittedValue = 0.0f;
        vssTxState[i].lastTxTime = 0;
        vssTxState[i].hasChanged = true;
        vssTxState[i].state = TX_STATE_CHANGED;
    }

    for (uint8_t i = 0; i < 8; ++i) {
        gpsTxState[i].lastTransmittedValue = 0.0f;
        gpsTxState[i].lastTxTime = 0;
        gpsTxState[i].hasChanged = true;
        gpsTxState[i].state = TX_STATE_CHANGED;
    }

    gearTxState.lastTransmittedValue = 0.0f;
    gearTxState.lastTxTime = 0;
    gearTxState.hasChanged = true;
    gearTxState.state = TX_STATE_CHANGED;

    Serial.println(F("MEGA_EPIC_CANBUS ready!"));
}

// ================================================================
// LOOP
// ================================================================

void loop()
{
    // ── CAN Error Recovery ──
    static unsigned long lastCanInitAttemptAt = 0;
    static uint8_t canInitFailureCycles = 0;

    if (!canReady) {
        unsigned long now = millis();
        if ((now - lastCanInitAttemptAt) >= CAN_REINIT_INTERVAL_MS) {
            lastCanInitAttemptAt = now;
            Serial.print(F("[CAN] Re-init attempt "));
            Serial.println(canInitFailureCycles + 1);
            if (initializeCanController()) {
                configureCANFilters();
                canReady = true;
                canInitFailureCycles = 0;
            } else {
                canInitFailureCycles++;
                if (canInitFailureCycles >= CAN_REINIT_MAX_CYCLES) {
                    Serial.println(F("[CAN] Max retries reached, CAN disabled"));
                    // Continue without CAN — GPS/AFR serial still works
                }
            }
        }
        delay(100);
        return;
    }

    // ── CAN RX ──
    {
        struct can_frame rxMsg;
        while (CAN.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
            handleCanFrame(rxMsg);
        }
    }

    // ── VSS Rate Calculation ──
    calculateVSSRates();

    unsigned long nowMs = millis();

    // ── Output Request ──
    {
        static unsigned long lastSlowOutRequestMs = 0;
        if (nowMs - lastSlowOutRequestMs >= SLOW_OUT_REQUEST_INTERVAL_MS) {
            lastSlowOutRequestMs = nowMs;
            sendVariableRequestFrame(VAR_HASH_OUT_SLOW);
        }
    }

    // ── GPS ──
    readGPSData();

    // ── Databox AFR ──
    databox.update();

    // ── Input Read ──
    {
        static unsigned long lastReadMs = 0;
        if (nowMs - lastReadMs >= TX_READ_INTERVAL_MS) {
            lastReadMs = nowMs;

            readAnalogInputs(currentAnalogValues);
            readDigitalInputs(&currentDigitalBits);
            readVSSInputs(currentVssValues);

            for (uint8_t i = 0; i < 16; ++i) {
                bool changed = hasAnalogChanged(i, currentAnalogValues[i]);
                updateTxState(&analogTxState[i], changed, nowMs);
            }

            bool digitalChanged = hasDigitalChanged(currentDigitalBits);
            updateTxState(&digitalTxState, digitalChanged, nowMs);

            for (uint8_t i = 0; i < 4; ++i) {
                bool changed = hasVSSChanged(i, currentVssValues[i]);
                updateTxState(&vssTxState[i], changed, nowMs);
            }
        }
    }

    // ── Smart TX: Analog ──
    for (uint8_t i = 0; i < 16; ++i) {
        transmitIfNeeded(&analogTxState[i], VAR_HASH_ANALOG[i], currentAnalogValues[i], nowMs);
    }

    // ── Smart TX: Digital ──
    transmitIfNeeded(&digitalTxState, VAR_HASH_DIGITAL_INPUT, (float)currentDigitalBits, nowMs);

    // ── Smart TX: VSS ──
    {
        const int32_t vssHashes[4] = {
            VAR_HASH_VSS_FRONT_LEFT, VAR_HASH_VSS_FRONT_RIGHT,
            VAR_HASH_VSS_REAR_LEFT, VAR_HASH_VSS_REAR_RIGHT
        };
        for (uint8_t i = 0; i < 4; ++i) {
            transmitIfNeeded(&vssTxState[i], vssHashes[i], currentVssValues[i], nowMs);
        }
    }

    // ── GPS TX ──
    transmitGPSIfNeeded();

    // ── Gear TX ──
    {
        GearState gear = readGearState();
        bool gearChanged = (gear.gear != (GearValue)(uint8_t)gearTxState.lastTransmittedValue);
        updateTxState(&gearTxState, gearChanged, nowMs);
        if (shouldTransmit(&gearTxState, nowMs) && gear.gear != kInvalid) {
            sendVariableSetFrameU32(VAR_HASH_GEAR, gear.gear);
            gearTxState.lastTransmittedValue = (float)gear.gear;
            gearTxState.lastTxTime = nowMs;
            gearTxState.hasChanged = false;
            gearTxState.state = TX_STATE_STABLE;
        }
    }

    // ── Wideband CAN TX (every 10ms) ──
    {
        static unsigned long lastWidebandSentAt = 0;
        if (nowMs - lastWidebandSentAt >= WIDEBAND_SEND_INTERVAL_MS) {
            lastWidebandSentAt = nowMs;
            const DataboxFrame &afr = databox.getFrame();
            sendWidebandFrame(afr.afr, afr.duty, afr.ur, databox.isConnected());
        }
    }
}
