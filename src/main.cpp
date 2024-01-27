#include <ESP8266WiFi.h>
#include <Arduino_JSON.h>
#include <ArduinoMqttClient.h>
#include "defines.h"
#include "helpers.h"
#include "autoupdate.h"

int current = 0;
unsigned long previousMillis = 0;

void setup()
{
  Serial.begin(115200);

  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB port only
  }
  boot();
  // Serial.print("getFreeHeap: ");
  // Serial.println(ESP.getFreeHeap());
  // Serial.print("getChipId: ");
  // Serial.println(ESP.getChipId());
  // Serial.print("getCoreVersion: ");
  // Serial.println(ESP.getCoreVersion());
  // Serial.print("getSdkVersion: ");
  // Serial.println(ESP.getSdkVersion());
  // Serial.print("getBootVersion: ");
  // Serial.println(ESP.getBootVersion());
  // Serial.print("getBootMode: ");
  // Serial.println(ESP.getBootMode());
  // Serial.print("getFlashChipId: ");
  // Serial.println(ESP.getFlashChipId());
  // Serial.print("getFlashChipVendorId: ");
  // Serial.println(ESP.getFlashChipVendorId());
  // Serial.print("getFlashChipRealSize: ");
  // Serial.println(ESP.getFlashChipRealSize());
  // Serial.print("getFlashChipSpeed: ");
  // Serial.println(ESP.getFlashChipSpeed());
  // Serial.print("getFreeSketchSpace: ");
  // Serial.println(ESP.getFreeSketchSpace());
  // Serial.print("getSketchMD5: ");
  // Serial.println(ESP.getSketchMD5());
}

void loop()
{
  if (getEnv(SYS_ACTION) == "WAIT")
  {
    digitalWrite(D6_PIN, LOW);
    println("Maintenance Mode.");
    delay(getEnv(LOOP_TIME).toInt());
    doUpdate();
  }
  else if (getEnv(SYS_ACTION) == "ON")
  {
    digitalWrite(D6_PIN, HIGH);
    println("User Power-On.");
    delay(getEnv(LOOP_TIME).toInt());
  }
  else if (getEnv(SYS_ACTION) == "OFF")
  {
    digitalWrite(D6_PIN, LOW);
    println("User Power-Off.");
    delay(getEnv(LOOP_TIME).toInt());
  }
  else if (current == 0 && digitalRead(D7_PIN) == LOW)
  {
    digitalWrite(D6_PIN, LOW);
    delay(100);
  }
  else if (digitalRead(D7_PIN) == HIGH)
  {
    println("Motion detected.");
    digitalWrite(D6_PIN, HIGH);
    current = getEnv(MAX_DELAY).toInt();
    delay(getEnv(LOOP_TIME).toInt());
  }
  else
  {
    current -= getEnv(LOOP_TIME).toInt();
    delay(getEnv(LOOP_TIME).toInt());
  }

  // to avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= (unsigned long)getEnv(LOOP_TIME).toInt())
  {
    // save the last time a message was sent
    previousMillis = currentMillis;

    if (!mqttClient.connected())
    {
      setEnv(SYS_ACTION, "FREE");

      if (!mqttClient.connect(getEnv(MQTT_BROKER).c_str(), getEnv(MQTT_PORT).toInt()))
      {
        print("MQTT connection failed! Error code = ");
        println(mqttClient.connectError());
      }
      else
      {
        // subscribe to a topic
        mqttClient.subscribe(getEnv(MQTT_SUB_TOPIC));
      }
    }

    // call poll() regularly to allow the library to receive MQTT messages and
    // send MQTT keep alive which avoids being disconnected by the broker
    mqttClient.poll();

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(getEnv(MQTT_PUB_TOPIC).c_str());
    mqttClient.print(JSON.stringify(buildHeartbeat()));
    mqttClient.endMessage();
  }
}
