#include <ESP8266httpUpdate.h>
#include "defines.h"
// #include "helpers.h"

// WiFiClient wifi;

void doUpdate()
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
