#include "DataboxManager.h"

#include <ctype.h>
#include <string.h>

DataboxManager::DataboxManager()
    : lineLength_(0), lastDataTime_(0), connected_(false),
      reconnectState_(SEND_STOP), stateTimer_(0), reconnectAttemptTime_(0) {
  memset(&currentFrame_, 0, sizeof(currentFrame_));
}

void DataboxManager::begin() {
  Serial.println(F("[Databox] Initializing on Serial3 (D14=TX, D15=RX) @ 57600"));
  Serial3.begin(DATABOX_BAUD);
}

void DataboxManager::update() {
  pumpData();

  const unsigned long now = millis();

  if (reconnectState_ == CONNECTED && connected_ &&
      (now - lastDataTime_ > DATABOX_TIMEOUT_MS)) {
    Serial.println(F("[Databox] Timeout! Starting reconnect..."));
    reconnectState_ = SEND_STOP;
    connected_ = false;
  }

  switch (reconnectState_) {
  case SEND_STOP:
    writeCommandLine(DATABOX_CMD_STOP);
    stateTimer_ = now;
    reconnectState_ = WAIT_STOP;
    break;

  case WAIT_STOP:
    if (now - stateTimer_ >= CMD_GAP_MS) {
      reconnectState_ = SEND_START_MAIN;
    }
    break;

  case SEND_START_MAIN:
    writeCommandLine(DATABOX_CMD_START_MAIN);
    stateTimer_ = now;
    reconnectState_ = WAIT_START_MAIN;
    break;

  case WAIT_START_MAIN:
    if (now - stateTimer_ >= CMD_GAP_MS) {
      reconnectState_ = SEND_START_STREAM;
    }
    break;

  case SEND_START_STREAM:
    writeCommandLine(DATABOX_CMD_START_STREAM);
    reconnectState_ = WAITING_RETRY;
    reconnectAttemptTime_ = now;
    break;

  case WAITING_RETRY:
    if (connected_) {
      Serial.println(F("[Databox] Connected & Streaming!"));
      reconnectState_ = CONNECTED;
    } else if (now - reconnectAttemptTime_ >= RECONNECT_RETRY_DELAY) {
      Serial.println(F("[Databox] Retry timeout - starting over..."));
      reconnectState_ = SEND_STOP;
    }
    break;

  case CONNECTED:
    break;

  case IDLE:
  default:
    break;
  }
}

bool DataboxManager::isConnected() const { return connected_; }

float DataboxManager::getAFRFloat() const {
  if (!connected_) return 0.0f;
  return currentFrame_.afr;
}

void DataboxManager::writeCommandLine(const char *cmd) {
  while (Serial3.available()) {
    (void)Serial3.read();
  }

  Serial3.print(cmd);
  Serial3.print(DATABOX_CMD_EOL);
  Serial3.flush();

  Serial.print(F("[Databox] TX CMD: "));
  Serial.println(cmd);
}

uint8_t DataboxManager::hexDigitValue(char c) {
  if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
  c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
  if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + c - 'A');
  return 0xFF;
}

bool DataboxManager::parseHex(const char *s, size_t len, uint32_t &value) {
  value = 0;
  for (size_t i = 0; i < len; i++) {
    const uint8_t d = hexDigitValue(s[i]);
    if (d == 0xFF) return false;
    value = (value << 4) | d;
  }
  return true;
}

bool DataboxManager::parseHexFrame(const char *line, DataboxFrame &out) {
  const char *p = line;
  while (*p == ' ' || *p == '\t') {
    ++p;
  }

  if (strncmp(p, "3628", 4) != 0) return false;

  const size_t len = strlen(p);
  if (len < 32) return false;

  uint32_t tmp = 0;

  if (!parseHex(p + 4, 4, tmp)) return false;
  out.afr = tmp / 100.0f;

  if (!parseHex(p + 8, 4, tmp)) return false;
  out.rpm = static_cast<uint16_t>(tmp);

  if (!parseHex(p + 12, 2, tmp)) return false;
  out.tps = static_cast<uint8_t>(tmp);

  if (!parseHex(p + 14, 3, tmp)) return false;
  out.eot = tmp / 10.0f;

  if (!parseHex(p + 17, 3, tmp)) return false;
  out.vbatt = tmp / 100.0f;

  if (!parseHex(p + 20, 3, tmp)) return false;
  out.ur = tmp / 1000.0f;

  if (!parseHex(p + 23, 4, tmp)) return false;
  out.duty = tmp / 100.0f;

  out.status[0] = p[27];
  out.status[1] = p[28];
  out.status[2] = '\0';

  if (!parseHex(p + 29, 3, tmp)) return false;
  out.urCal = tmp / 1000.0f;

  return true;
}

void DataboxManager::pumpData() {
  int bytesProcessed = 0;
  const int maxBytesPerLoop = 64;

  while (Serial3.available() && bytesProcessed < maxBytesPerLoop) {
    const char ch = static_cast<char>(Serial3.read());
    bytesProcessed++;

    if (ch == '\r' || ch == '\n') {
      if (lineLength_ == 0) continue;

      lineBuffer_[lineLength_] = '\0';

      DataboxFrame parsed{};
      if (parseHexFrame(lineBuffer_, parsed)) {
        if (parsed.afr >= 7.0f && parsed.afr <= 80.0f) {
          currentFrame_ = parsed;
          lastDataTime_ = millis();

          if (!connected_) {
            connected_ = true;
            Serial.println(F("[Databox] Connected!"));
          }
        }
      }

      lineLength_ = 0;
      continue;
    }

    if (lineLength_ < (DATABOX_LINE_BUFFER_SIZE - 1)) {
      lineBuffer_[lineLength_++] = ch;
    } else {
      lineLength_ = 0;
    }
  }
}
