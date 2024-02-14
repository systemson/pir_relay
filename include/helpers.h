
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266httpUpdate.h>
#include <WebSerial.h>
#include "defines.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
AsyncWebServer server(80);
int current = 0;

String CONFIG[CONFIG_SIZE];

void setEnv(const int &key, const String &value)
{
  CONFIG[key] = value;
}

String getEnv(const int &key)
{
  return CONFIG[key];
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

  Serial.print("Received command: ");
  Serial.println(command);

  if (command == "SET")
  {
    const int key = (int)json["key"];
    const String value = (String)json["value"];
    Serial.print("Setting [");
    Serial.print(key);
    Serial.print(":");
    Serial.print(value);
    Serial.println("]");
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
  json["component_type"] = type;
  json["description"] = description;

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(getEnv(MQTT_REG_TOPIC).c_str());
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
  setEnv(SYS_MAINTENANCE, "FALSE");
  setEnv(SYS_GROUP, "dpena");
  setEnv(SYS_ACTION, "FREE");
  setEnv(SYS_HARD_RESET, "FALSE");

  // setEnv(WIFI_SSID, "Amilcar2.4G");
  // setEnv(WIFI_PASS, "Gigi.2022");

  setEnv(WIFI_SSID, "Deivi");
  setEnv(WIFI_PASS, "Amilcar.1");
  setEnv(MAC_ADDRESS, WiFi.macAddress());

  // setEnv(MQTT_BROKER, "192.168.1.11");
  setEnv(MQTT_BROKER, "192.168.69.42");
  setEnv(MQTT_PORT, "1883");
  setEnv(MQTT_PUB_TOPIC, "arduino/heartbeat");
  setEnv(MQTT_SUB_TOPIC, "arduino/" + getEnv(SYS_GROUP) + "/" + getEnv(MAC_ADDRESS) + "/read");
  setEnv(MQTT_CONF_TOPIC, "arduino/" + getEnv(SYS_GROUP) + "/" + getEnv(MAC_ADDRESS) + "/response");
  setEnv(MQTT_REG_TOPIC, "arduino/component/register");
  setEnv(MQTT_READ_TOPIC, "arduino/component/reading");

  setEnv(MAX_DELAY, "10000");
  setEnv(LOOP_TIME, "1000");
}

void connectWiFi()
{
  // Connect to WiFi
  WiFi.begin(getEnv(WIFI_SSID), getEnv(WIFI_PASS));

  WiFi.mode(WIFI_STA);

  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  // The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  const String ipAddress = WiFi.localIP().toString();
  Serial.print("Network IP address: ");
  Serial.println(ipAddress);
  setEnv(IP_ADDRESS, ipAddress);

  // print("Connection RSSI: ");
  // println(WiFi.RSSI().toString());
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

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.print(getEnv(MQTT_BROKER));
  Serial.print(":");
  Serial.println(getEnv(MQTT_PORT));

  if (!mqttClient.connect(getEnv(MQTT_BROKER).c_str(), getEnv(MQTT_PORT).toInt()))
  {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    delay(3000);
    ESP.restart();
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println("");

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  Serial.print("Subscribing to topic: ");
  Serial.println(getEnv(MQTT_SUB_TOPIC));
  Serial.println("");

  // subscribe to a topic
  mqttClient.subscribe(getEnv(MQTT_SUB_TOPIC));
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "application/json", "{\"message\": \"Not found\"}");
}

void homeRoute(AsyncWebServerRequest *request)
{
  request->send(200, "application/json", "{\"message\":\"Running\"}");
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

void setRoutes()
{
  server.onNotFound(notFound);

  server.on("/", HTTP_GET, homeRoute);

  server.on("/info", HTTP_GET, infoRoute);

  server.on("/config", HTTP_GET, getConfigRoute);

  server.on("/config", HTTP_POST, setConfigRoute);

  WebSerial.begin(&server);
  // WebSerial.msgCallback(recvMsg);
  server.begin();
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

void boot()
{
  Serial.println("");
  Serial.println("Starting NodeMCU v1.0 Rev3");

  if (getEnv(SYS_HARD_RESET) != "FALSE")
  {
    Serial.println("Restoring factory defaults");
    hardReset();
  }

  connectWiFi();

  connectMqtt();

  setRoutes();
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
    Serial.println("Your code is up to date!");
    delay(LOOP_TIME);
    setEnv(SYS_ACTION, "FREE");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
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

  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alive which avoids being disconnected by the broker
  mqttClient.poll();

  sendHeartBeat();
}

void mainLoop()
{
  const int loopTime = getEnv(LOOP_TIME).toInt();
  const String sysAction = getEnv(SYS_ACTION);

  if (sysAction == "WAIT")
  {
    digitalWrite(D6, LOW);
    println("Maintenance Mode.");
    tryUpdate();
  }
  else if (sysAction == "ON")
  {
    digitalWrite(D6, HIGH);
    println("User Power-On.");
  }
  else if (sysAction == "OFF")
  {
    digitalWrite(D6, LOW);
    println("User Power-Off.");
  }
  else if (current == 0 && digitalRead(D7) == LOW)
  {
    digitalWrite(D6, LOW);
  }
  else if (digitalRead(D7) == HIGH)
  {
    println("Motion detected.");
    digitalWrite(D6, HIGH);
    current = getEnv(MAX_DELAY).toInt();
  }
  else
  {
    current -= loopTime;
  }
}
unsigned long noDelayLoop(unsigned long previousMillis, unsigned long loopTime, void (*callback)(void))
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= loopTime)
  {
    // save the last time a message was sent
    previousMillis = currentMillis;

    callback();
  }

  return previousMillis;
}

