#include <Arduino.h>
#include <EEPROM.h>
#include "helpers.h"
#include "implement.h"

unsigned long previousMillis = 0;

void setup()
{
  Serial.begin(115200);

  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB port only
  }

  boot();

  sendHeartBeat();
  turnOn();
}

void loop()
{
  noDelayLoop(0, 1000, &mainLoop);
  noDelayLoop(1, getEnv(HEARTBEAT_TIME).toInt(), &loopHeartBeat);
}
