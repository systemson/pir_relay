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
    //
  }

  boot();

  sendHeartBeat();
  turnOn();
}

void loop()
{
  while (getEnv(SYS_ACTION) == "WAIT")
  {
    Serial.println(F("Maintenance Mode."));
    delay(1000);
  }

  noDelayLoop(0, 1000, &mainLoop);
  noDelayLoop(1, getEnv(HEARTBEAT_TIME).toInt(), &loopHeartBeat);
}
