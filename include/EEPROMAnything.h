#pragma once
#include <EEPROM.h>
#include <Arduino.h>

unsigned long long EEPROM_SIZE = 512;

void EEPROM_clear()
{
  for (size_t i = 0; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, 255);
  }
}

void EEPROM_write(const char *value)
{
  for (size_t i = 0; i < strlen(value); i++)
  {
    EEPROM.write(i, value[i]);
  }
}

char *EEPROM_read()
{
  char *result = new char[EEPROM_SIZE + 1];

  for (size_t i = 0; i < EEPROM_SIZE; i++)
  {
    uint8_t bit = EEPROM.read(i);

    if (bit != 255)
    {
      result[i] = bit;
    }
  }

  result[EEPROM_SIZE + 1] = '\0';

  return result;
}
