#include <ESP8266WiFi.h>
#include <Arduino_JSON.h>
#include <ArduinoMqttClient.h>
#include "defines.h"
#include "helpers.h"

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
  registerComponent("HC-SR501", "LIVING ROOM");
  pinMode(D7, INPUT);
  pinMode(D6, OUTPUT);
}

void loop()
{
  const int loopTime = getEnv(LOOP_TIME).toInt();
  const String sysAction = getEnv(SYS_ACTION);
  
  noDelayLoop(previousMillis, loopTime, mainLoop);
  previousMillis = noDelayLoop(previousMillis, loopTime, loopHeartBeat);
}
