#pragma once
#include "helpers.h"
#include "defines.h"

int current = 0;

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
