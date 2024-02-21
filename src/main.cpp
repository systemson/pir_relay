#include <Arduino.h>
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
  const int loopTime = getEnv(LOOP_TIME).toInt();

  noDelayLoop(0, loopTime, &mainLoop);
  noDelayLoop(1, loopTime * 10, &loopHeartBeat);
}
