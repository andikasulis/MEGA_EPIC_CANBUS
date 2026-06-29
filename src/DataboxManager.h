#ifndef DATABOXMANAGER_H
#define DATABOXMANAGER_H

#include <Arduino.h>

#define DATABOX_BAUD 57600
#define DATABOX_TIMEOUT_MS 5000
#define DATABOX_LINE_BUFFER_SIZE 96
#define DATABOX_CMD_STOP "3649"
#define DATABOX_CMD_START_MAIN "3640;00"
#define DATABOX_CMD_START_STREAM "3648"
#define DATABOX_CMD_EOL "\r\n"

struct DataboxFrame {
  float afr;
  uint16_t rpm;
  uint8_t tps;
  float eot;
  float vbatt;
  float ur;
  float duty;
  char status[3];
  float urCal;
};

class DataboxManager {
public:
  DataboxManager();

  void begin();
  void update();
  bool isConnected() const;
  float getAFRFloat() const;
  const DataboxFrame &getFrame() const { return currentFrame_; }

private:
  char lineBuffer_[DATABOX_LINE_BUFFER_SIZE];
  size_t lineLength_;
  DataboxFrame currentFrame_;
  unsigned long lastDataTime_;
  bool connected_;

  enum ReconnectState {
    IDLE,
    SEND_STOP,
    WAIT_STOP,
    SEND_START_MAIN,
    WAIT_START_MAIN,
    SEND_START_STREAM,
    CONNECTED,
    WAITING_RETRY
  };

  ReconnectState reconnectState_;
  unsigned long stateTimer_;
  unsigned long reconnectAttemptTime_;
  static const unsigned long RECONNECT_RETRY_DELAY = 2000;
  static const unsigned long CMD_GAP_MS = 50;

  void writeCommandLine(const char *cmd);
  uint8_t hexDigitValue(char c);
  bool parseHex(const char *s, size_t len, uint32_t &value);
  bool parseHexFrame(const char *line, DataboxFrame &frame);
  void pumpData();
};

#endif
