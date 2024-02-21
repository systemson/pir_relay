#pragma once
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266httpUpdate.h>
#include <WebSerial.h>
#include "defines.h"
#include "homepage.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
AsyncWebServer server(80);

String CONFIG[CONFIG_SIZE];

void turnOff();

void setEnv(const int &key, const String &value)
{
  CONFIG[key] = value;
}

String getEnv(const int &key)
{
  return CONFIG[key];
}

void print(String msg)
{
  Serial.print(msg);
  WebSerial.print(msg);
}

void print(int msg)
{
  Serial.print(msg);
  WebSerial.print(msg);
}

void println(String msg)
{
  Serial.println(msg);
  WebSerial.println(msg);
}

void println(int msg)
{
  Serial.println(msg);
  WebSerial.println(msg);
}

int readSensor(uint8_t sensor)
{
  digitalWrite(sensor, HIGH);
  delay(10);
  const int val = analogRead(A0);
  digitalWrite(sensor, LOW);
  return val;
}

void onMqttMessage(int messageSize)
{
  String message = "";

  while (mqttClient.available())
  {
    message += (char)mqttClient.read();
  }

  JSONVar json = JSON.parse(message);
  const String command = (String)json["command"];

  Serial.print(F("Received command: "));
  Serial.println(command);

  if (command == "SET")
  {
    const int key = (int)json["key"];
    const String value = (String)json["value"];
    Serial.print(F("Setting ["));
    Serial.print(key);
    Serial.print(F(":"));
    Serial.print(value);
    Serial.println(F("]"));
    setEnv(key, value);
  }
  else if (command == "GET")
  {
    const int key = (int)json["key"];
    JSONVar response;

    response["key"] = key;
    response["value"] = getEnv(key);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(getEnv(MQTT_CONF_TOPIC));
    mqttClient.print(JSON.stringify(response));
    mqttClient.endMessage();
  }
  else
  {
    setEnv(SYS_ACTION, command);
  }
}

JSONVar buildHeartbeat(const boolean &IsOnline = true)
{
  JSONVar json;

  json["mac_address"] = getEnv(MAC_ADDRESS);
  json["ip_address"] = getEnv(IP_ADDRESS);
  json["group_name"] = getEnv(SYS_GROUP);
  json["is_online"] = IsOnline;
  json["running_since"] = millis();
  json["current_command"] = getEnv(SYS_ACTION);
  json["board"] = BOARD_NAME;
  json["firmware_version"] = FIRMWARE_VERSION;

  return json;
}

void registerComponent(String type, String description)
{
  JSONVar json;

  json["mac_address"] = getEnv(MAC_ADDRESS);
  json["component_name"] = type;
  json["description"] = description;

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(getEnv(MQTT_REG_TOPIC).c_str());
  mqttClient.print(JSON.stringify(json));
  mqttClient.endMessage();
}

void sendReading(String type, String description, String value, String status)
{
  JSONVar json;

  json["mac_address"] = getEnv(MAC_ADDRESS);
  json["component_name"] = type;
  json["description"] = description;
  json["value"] = value;
  json["status"] = status;

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(getEnv(MQTT_READ_TOPIC).c_str());
  mqttClient.print(JSON.stringify(json));
  mqttClient.endMessage();
}

void sendHeartBeat()
{
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(getEnv(MQTT_PUB_TOPIC).c_str());
  mqttClient.print(JSON.stringify(buildHeartbeat()));
  mqttClient.endMessage();
}
void hardReset()
{
  // setEnv(SYS_MAINTENANCE, "FALSE");
  setEnv(SYS_GROUP, "dpena");
  setEnv(SYS_ACTION, "FREE");
  setEnv(SYS_HARD_RESET, "FALSE");

  // setEnv(WIFI_SSID, "Amilcar2.4G");
  // setEnv(WIFI_PASS, "Gigi.2022");

  setEnv(WIFI_SSID, "Deivi");
  setEnv(WIFI_PASS, "Amilcar.1");
  setEnv(MAC_ADDRESS, WiFi.macAddress());

  // setEnv(MQTT_BROKER, "192.168.1.11");
  setEnv(MQTT_BROKER, "192.168.15.42");
  setEnv(MQTT_PORT, "1883");
  setEnv(MQTT_PUB_TOPIC, "arduino/heartbeat");
  setEnv(MQTT_SUB_TOPIC, "arduino/" + getEnv(SYS_GROUP) + "/" + getEnv(MAC_ADDRESS) + "/inbox");
  setEnv(MQTT_CONF_TOPIC, "arduino/" + getEnv(SYS_GROUP) + "/" + getEnv(MAC_ADDRESS) + "/outbox");
  setEnv(MQTT_REG_TOPIC, "arduino/component/register");
  setEnv(MQTT_READ_TOPIC, "arduino/component/reading");

  setEnv(LOOP_TIME, "1000");
}

boolean connectWiFi(boolean accessPoint = false)
{
  WiFi.mode(WIFI_AP_STA);

  // Start Access Point
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  Serial.print(F("Access Point name: "));
  Serial.println(WIFI_AP_SSID);

  // Print the Access Point IP address
  const String apIP = WiFi.softAPIP().toString();
  Serial.print(F("Access Point IP address: "));
  Serial.println(apIP);

  // Connect to WiFi
  WiFi.begin(getEnv(WIFI_SSID), getEnv(WIFI_PASS));

  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(F("."));
  }

  // The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // print a new line, then print WiFi connected and the IP address
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Print the Local IP address
  const String localIP = WiFi.localIP().toString();
  Serial.print(F("Local IP address: "));
  Serial.println(localIP);
  setEnv(IP_ADDRESS, localIP);

  // print("Connection RSSI: ");
  // println(WiFi.RSSI().toString());

  return accessPoint;
}

void connectMqtt()
{
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId(getEnv(MAC_ADDRESS));

  // set a will message, used by the broker when the connection dies unexpectedly
  // you must know the size of the message beforehand, and it must be set before connecting
  String willPayload = JSON.stringify(buildHeartbeat(false));

  mqttClient.beginWill(getEnv(MQTT_PUB_TOPIC), willPayload.length(), true, 1);
  mqttClient.print(willPayload);
  mqttClient.endWill();

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword("username", "password");

  Serial.print(F("Attempting to connect to the MQTT broker: "));
  Serial.print(getEnv(MQTT_BROKER));
  Serial.print(F(":"));
  Serial.println(getEnv(MQTT_PORT));

  if (!mqttClient.connect(getEnv(MQTT_BROKER).c_str(), getEnv(MQTT_PORT).toInt()))
  {
    Serial.print(F("MQTT connection failed! Error code = "));
    Serial.println(mqttClient.connectError());
    delay(3000);
    ESP.restart();
  }

  Serial.println(F("You're connected to the MQTT broker!"));
  Serial.println();

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  Serial.print(F("Subscribing to topic: "));
  Serial.println(getEnv(MQTT_SUB_TOPIC));
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(getEnv(MQTT_SUB_TOPIC));
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "application/json", "{\"message\": \"Not found\"}");
}

void homeRoute(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", INDEX_HTML);
}
void infoRoute(AsyncWebServerRequest *request)
{
  request->send(200, "application/json", JSON.stringify(buildHeartbeat()));
}
void getConfigRoute(AsyncWebServerRequest *request)
{
  if (!request->hasParam("key"))
  {
    return request->send(422, "application/json", "{\"message\": \"Invalid request\"}");
  }

  AsyncWebParameter *key = request->getParam("key");

  JSONVar response;

  response["key"] = key->value();
  response["value"] = getEnv(key->value().toInt());

  request->send(200, "application/json", JSON.stringify(response));
}
void setConfigRoute(AsyncWebServerRequest *request)
{
  if (!request->hasParam("key", true) || !request->hasParam("value", true))
  {
    return request->send(422, "application/json", "{\"message\": \"Invalid request\"}");
  }

  AsyncWebParameter *key = request->getParam("key", true);
  AsyncWebParameter *value = request->getParam("value", true);

  Serial.println(key->value());
  Serial.println(value->value());

  setEnv(key->value().toInt(), value->value());

  JSONVar response;

  response["key"] = key->value();
  response["value"] = value->value();

  request->send(200, "application/json", JSON.stringify(response));
}

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML><html><head>
      <title>ESP Input Form</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      </head><body>
      <form action="/get">
        input1: <input type="text" name="input1">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        input2: <input type="text" name="input2">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        input3: <input type="text" name="input3">
        <input type="submit" value="Submit">
      </form>
    </body></html>)rawliteral";

void setRoutes()
{
  WebSerial.begin(&server);
  // WebSerial.msgCallback(recvMsg);

  server.onNotFound(notFound);

  server.on("/config", HTTP_GET, getConfigRoute);
  server.on("/config", HTTP_POST, setConfigRoute);
  server.on("/", HTTP_GET, homeRoute);
  server.on("/info", HTTP_GET, infoRoute);

  server.begin();
}
void boot()
{
  Serial.println();
  Serial.println(F("Starting NodeMCU v1.0 Rev3"));
  Serial.print(F(BOARD_NAME));
  Serial.print(F(" | "));
  Serial.println(F(FIRMWARE_VERSION));
  Serial.println();

  if (getEnv(SYS_HARD_RESET) != "FALSE")
  {
    Serial.println(F("Restoring factory defaults."));
    hardReset();
  }

  setRoutes();

  connectWiFi();

  connectMqtt();
}
void tryUpdate()
{
  // The line below is optional. It can be used to blink the LED on the board during flashing
  // The LED will be on during download of one buffer of data from the network. The LED will
  // be off during writing that buffer to flash
  // On a good connection the LED should flash regularly. On a bad connection the LED will be
  // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
  // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  // Add optional callback notifiers
  // ESPhttpUpdate.onStart(update_started);
  // ESPhttpUpdate.onEnd(update_finished);
  // ESPhttpUpdate.onProgress(update_progress);
  // ESPhttpUpdate.onError(update_error);

  t_httpUpdate_return ret = ESPhttpUpdate.update(wifiClient, getEnv(UPDATE_URL), FIRMWARE_VERSION);
  // Or:
  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    setEnv(SYS_ACTION, "FREE");
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println(F("Your code is up to date!"));
    delay(LOOP_TIME);
    setEnv(SYS_ACTION, "FREE");
    break;

  case HTTP_UPDATE_OK:
    Serial.println(F("HTTP_UPDATE_OK"));
    delay(LOOP_TIME); // Wait a second and restart
    setEnv(SYS_ACTION, "FREE");
    ESP.restart();
    break;
  }
}

void loopHeartBeat()
{

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

  if (getEnv(SYS_ACTION) == "WAIT")
  {
    println("Maintenance Mode.");
    turnOff();
    tryUpdate();
  }

  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alive which avoids being disconnected by the broker
  mqttClient.poll();

  sendHeartBeat();
}

unsigned long noDelayCounter[3] = {};

void noDelayLoop(int loopNum, unsigned long loopTime, void (*callback)(void))
{
  if (!noDelayCounter[loopNum])
  {
    noDelayCounter[loopNum] = 0;
  }

  unsigned long previousMillis = noDelayCounter[loopNum];

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= loopTime)
  {
    callback();
    noDelayCounter[loopNum] = currentMillis;
  }
}
