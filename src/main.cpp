/*
  NANO_EPIC_CANBUS - Minimal Arduino Nano CAN sender for quickshifter + clutch.
  Sends one analog input and one digital input to epicEFI ECU via EPIC_CAN_BUS.
  Board: Arduino Nano (ATmega328P) + MCP2515 CAN shield.
*/

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

// MCP2515 crystal: MCP_8MHZ for generic modules, MCP_16MHZ for Seeed shields.
#define BOARD_CAN_CLOCK MCP_8MHZ

// MCP2515 chip select (change if your shield uses a different pin).
#define SPI_CS_PIN 10

MCP2515 CAN(SPI_CS_PIN);

// ECU / EPIC CAN IDs
#define ECU_CAN_ID 1
#define CAN_ID_VAR_REQUEST       (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE      (0x720 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET      (0x780 + ECU_CAN_ID)

// Pin mapping (Arduino Nano ATmega328P)
// SPI is on D10..D13; D10 is MCP2515 CS. D0/D1 are Serial.
// Safe choices: A0..A7 for ADC, D2..D9 and A6/A7 for digital inputs.
#define QUICKSHIFTER_ADC_PIN A0  // ADC0
#define CLUTCH_DIGITAL_PIN   4   // INPUT_PULLUP: LOW = clutch active

// The firmware exposes the clutch as bit 0 of the MEGA_EPIC_1_D22_D37 packed
// digital word. On the original Mega2560 board bit 0 maps to physical D22.
// On this Nano build we reuse the same EPIC variable so the ECU side stays
// unchanged; only the local Arduino pin is different.
#define CLUTCH_BIT_INDEX 0

// Variable hashes from variables.json / ECU.
// See docs/CAN_VARIABLE_MAP.md for valid EPIC CAN variables.
#define VAR_HASH_QUICKSHIFTER 595545759L   // MEGA_EPIC_1_A0
#define VAR_HASH_CLUTCH       2138825443L  // MEGA_EPIC_1_D22_D37

// Smart TX parameters
#define TX_READ_INTERVAL_MS 10
#define TX_INTERVAL_FAST_MS 25
#define TX_INTERVAL_SLOW_MS 500
#define TX_ANALOG_THRESHOLD 2.0f

// ADC scaling: the ECU outputChannels stores MEGA_EPIC_1_A0 as U16 raw counts.
// The EPIC variable_set frame uses float32, so send raw 0..1023 counts.
#define ADC_SEND_RAW_COUNTS 1

// Interactive serial logging
#define SERIAL_LOG_INTERVAL_MS 500
#define CAN_RX_LOG_INTERVAL_MS 250
#define CAN_RX_LOG_MAX_PER_INTERVAL 10

struct TxChannelState {
    float lastValue;
    unsigned long lastTxMs;
    bool changed;
    bool stable;
};

static TxChannelState adcState;
static TxChannelState clutchState;

static float currentAdc = 0.0f;
static uint8_t currentClutch = 0; // 0 or 1

static struct can_frame txMsg;

static inline void writeInt32BigEndian(int32_t value, uint8_t* out) {
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static inline void writeFloat32BigEndian(float value, uint8_t* out) {
    union { float f; uint32_t u; } conv = { value };
    out[0] = (uint8_t)(conv.u >> 24);
    out[1] = (uint8_t)(conv.u >> 16);
    out[2] = (uint8_t)(conv.u >> 8);
    out[3] = (uint8_t)conv.u;
}

static uint32_t canTxOkCount = 0;
static uint32_t canTxErrCount = 0;

// Last transmitted frame details for serial logging.
static int32_t lastTxHash = 0;
static float   lastTxValue = 0.0f;

static inline uint8_t sendVariableSetFrame(int32_t varHash, float value) {
    txMsg.can_id  = CAN_ID_VARIABLE_SET;
    txMsg.can_dlc = 8;
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    writeFloat32BigEndian(value, &txMsg.data[4]);
    uint8_t err = CAN.sendMessage(&txMsg);
    if (err == MCP2515::ERROR_OK) {
        canTxOkCount++;
        lastTxHash = varHash;
        lastTxValue = value;
    } else {
        canTxErrCount++;
    }
    return err;
}

// Pack digital inputs into the MEGA_EPIC_1_D22_D37 bitfield word.
// Bit 0 = clutch (LOW active, INPUT_PULLUP).
static inline uint16_t packDigitalInputs(uint8_t clutchActive) {
    uint16_t bits = 0;
    if (clutchActive) {
        bits |= (1u << CLUTCH_BIT_INDEX);
    }
    return bits;
}

static void configureCANFilters() {
    // Promiscuous RX: accept all standard 11-bit frames so we can log/sniff
    // traffic such as Haltech broadcasts. The EPIC response filter is removed.
    CAN.setFilterMask(MCP2515::MASK0, false, 0x000);
    CAN.setFilterMask(MCP2515::MASK1, false, 0x000);
    CAN.setFilter(MCP2515::RXF0, false, 0x000);
    CAN.setFilter(MCP2515::RXF1, false, 0x000);
    CAN.setFilter(MCP2515::RXF2, false, 0x000);
    CAN.setFilter(MCP2515::RXF3, false, 0x000);
    CAN.setFilter(MCP2515::RXF4, false, 0x000);
    CAN.setFilter(MCP2515::RXF5, false, 0x000);
}

static bool shouldTransmit(TxChannelState* s, unsigned long nowMs) {
    if (s->changed) {
        return (nowMs - s->lastTxMs) >= TX_INTERVAL_FAST_MS;
    }
    return (nowMs - s->lastTxMs) >= TX_INTERVAL_SLOW_MS;
}

static inline void markChanged(TxChannelState* s, bool changed) {
    if (changed) {
        s->changed = true;
        s->stable = false;
    }
}

static inline void updateAfterTx(TxChannelState* s, float value, unsigned long nowMs) {
    s->lastValue = value;
    s->lastTxMs  = nowMs;
    s->changed   = false;
    s->stable    = true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial); // Remove for stand-alone (battery) operation

    Serial.println(F("NANO_EPIC_CANBUS starting"));
    Serial.print(F("CAN bitrate=500KBPS crystal="));
    Serial.println(BOARD_CAN_CLOCK == MCP_16MHZ ? F("16MHZ") : F("8MHZ"));

    CAN.reset();
    CAN.setBitrate(CAN_500KBPS, BOARD_CAN_CLOCK);
    configureCANFilters();
    CAN.setNormalMode();
    Serial.println(F("CAN init done"));

    pinMode(QUICKSHIFTER_ADC_PIN, INPUT);
    pinMode(CLUTCH_DIGITAL_PIN, INPUT_PULLUP);

    adcState.lastValue = 0.0f;
    adcState.lastTxMs  = 0;
    adcState.changed   = true;
    adcState.stable    = false;

    clutchState.lastValue = 0.0f;
    clutchState.lastTxMs  = 0;
    clutchState.changed   = true;
    clutchState.stable    = false;
}

static void logCanFrame(const struct can_frame& frame) {
    Serial.print(F("RX "));
    Serial.print(frame.can_id, HEX);
    Serial.print(F(" ["));
    Serial.print(frame.can_dlc);
    Serial.print(F("] "));
    for (uint8_t i = 0; i < frame.can_dlc; ++i) {
        if (frame.data[i] < 0x10) Serial.print('0');
        Serial.print(frame.data[i], HEX);
        if (i < frame.can_dlc - 1) Serial.print(' ');
    }
    Serial.println();
}

void loop() {
    unsigned long nowMs = millis();

    static unsigned long lastRxLogMs = 0;
    static uint8_t rxLogCount = 0;
    struct can_frame rxMsg;
    while (CAN.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
        if (rxLogCount < CAN_RX_LOG_MAX_PER_INTERVAL &&
            (nowMs - lastRxLogMs) >= CAN_RX_LOG_INTERVAL_MS) {
            logCanFrame(rxMsg);
            rxLogCount++;
        }
    }
    if ((nowMs - lastRxLogMs) >= CAN_RX_LOG_INTERVAL_MS) {
        lastRxLogMs = nowMs;
        rxLogCount = 0;
    }

    nowMs = millis();

    static unsigned long lastReadMs = 0;
    if (nowMs - lastReadMs >= TX_READ_INTERVAL_MS) {
        lastReadMs = nowMs;

        currentAdc = (float)analogRead(QUICKSHIFTER_ADC_PIN);
        currentClutch = (digitalRead(CLUTCH_DIGITAL_PIN) == LOW) ? 1u : 0u;

        float adcDiff = currentAdc - adcState.lastValue;
        if (adcDiff < 0.0f) adcDiff = -adcDiff;
        markChanged(&adcState, adcDiff >= TX_ANALOG_THRESHOLD);

        uint16_t packedDigital = packDigitalInputs(currentClutch);
        markChanged(&clutchState, packedDigital != (uint16_t)clutchState.lastValue);
    }

    static uint8_t lastAdcErr = 0;
    static uint8_t lastClutchErr = 0;

    if (shouldTransmit(&adcState, nowMs)) {
        lastAdcErr = sendVariableSetFrame(VAR_HASH_QUICKSHIFTER, currentAdc);
        updateAfterTx(&adcState, currentAdc, nowMs);
    }

    if (shouldTransmit(&clutchState, nowMs)) {
        uint16_t packedDigital = packDigitalInputs(currentClutch);
        lastClutchErr = sendVariableSetFrame(VAR_HASH_CLUTCH, (float)packedDigital);
        updateAfterTx(&clutchState, (float)packedDigital, nowMs);
    }

    static unsigned long lastLogMs = 0;
    if (nowMs - lastLogMs >= SERIAL_LOG_INTERVAL_MS) {
        lastLogMs = nowMs;
        Serial.print(F("ADC="));
        Serial.print(currentAdc, 0);
        Serial.print(F(" CLUTCH="));
        Serial.print(currentClutch);
        Serial.print(F(" DIGITAL="));
        Serial.print(packDigitalInputs(currentClutch), BIN);
        Serial.print(F(" ADC_CHG="));
        Serial.print(adcState.changed ? '1' : '0');
        Serial.print(F(" CLUTCH_CHG="));
        Serial.print(clutchState.changed ? '1' : '0');
        Serial.print(F(" TX_OK="));
        Serial.print(canTxOkCount);
        Serial.print(F(" TX_ERR="));
        Serial.print(canTxErrCount);
        Serial.print(F(" ERR_ADC="));
        Serial.print(lastAdcErr);
        Serial.print(F(" ERR_CLUTCH="));
        Serial.print(lastClutchErr);
        Serial.print(F(" LAST_HASH="));
        Serial.print(lastTxHash, HEX);
        Serial.print(F(" LAST_VAL="));
        Serial.println(lastTxValue, 0);
    }
}
