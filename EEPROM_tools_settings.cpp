#include <Arduino.h>
#include <I2C_eeprom.h>
#include "settings.h"

extern I2C_eeprom EEPROM;


uint8_t getDayDotsBrightness(void)
{
    return EEPROM.readByte(DOTS_DEFAULT_BRIGHTNESS_EEPROM_ADDRESS);
}

void setDayDotsBrightness(uint8_t dayBrightness)
{
    EEPROM.updateByte(DOTS_DEFAULT_BRIGHTNESS_EEPROM_ADDRESS, dayBrightness);
}

uint8_t getNightDotsBrightness(void)
{
    return EEPROM.readByte(DOTS_NIGHT_BRIGHTNESS_EEPROM_ADDRESS);
}

void setNightDotsBrightness(uint8_t nightBrightness)
{
    EEPROM.updateByte(DOTS_NIGHT_BRIGHTNESS_EEPROM_ADDRESS, nightBrightness);
}

uint8_t getDayBrightness(void)
{
    return EEPROM.readByte(DEFAULT_BRIGHTNESS_EEPROM_ADDRESS);
}

void setDayBrightness(uint8_t dayBrightness)
{
    EEPROM.updateByte(DEFAULT_BRIGHTNESS_EEPROM_ADDRESS, dayBrightness);
}

uint8_t getNightBrightness(void)
{
    return EEPROM.readByte(NIGHT_BRIGHTNESS_EEPROM_ADDRESS);
}

void setNightBrightness(uint8_t nightBrightness)
{
    EEPROM.updateByte(NIGHT_BRIGHTNESS_EEPROM_ADDRESS, nightBrightness);
}

void setGMTOffset(uint32_t offset)
{
    EEPROM.updateByte(GMT_OFFSET_HOURS_EEPROM_ADDRESS, (uint8_t)(offset / 60 / 60)); 
    EEPROM.updateByte(GMT_OFFSET_MINUTES_EEPROM_ADDRESS, (uint8_t)(offset / 60 % 60));
}

uint32_t getGMTOffset(void)
{
    uint32_t seconds = (uint32_t)EEPROM.readByte(GMT_OFFSET_MINUTES_EEPROM_ADDRESS) * 60UL;
    if (!(seconds % 15))
    {
      seconds = seconds % 15;
    }
    return (uint32_t)EEPROM.readByte(GMT_OFFSET_HOURS_EEPROM_ADDRESS) * 60UL * 60UL + seconds;
}

uint32_t getIPAddress(void)
{
    uint32_t address; 
    EEPROM.readBlock(DEFAULT_IP_ADDRESS_ADDRESS, (uint8_t *)&address, sizeof(address));
    return address;
}

void setIPAddress(uint32_t address)
{ 
    EEPROM.updateBlock(DEFAULT_IP_ADDRESS_ADDRESS, (uint8_t *)&address, sizeof(address));
}

void EEPROMValuesInit(bool force = false)
{
  if ((EEPROM.readByte((uint16_t)INIT_ADDR) != INIT_KEY) || force)  // первый запуск или принудительная очистка
  {
    EEPROM.updateByte((uint16_t)INIT_ADDR, INIT_KEY);    // записали ключ
    // записали стандартное значение яркости
    // в данном случае это значение переменной, объявленное выше
    setDayBrightness(DEFAULT_BRIGHTNESS);
    setNightBrightness(NIGHT_BRIGHTNESS);
    setGMTOffset(GMT_SECONDS_OFFSET);
    setDayDotsBrightness(DOTS_DAY_BRIGHTNESS);
    setNightDotsBrightness(DOTS_NIGHT_BRIGHTNESS);
    setIPAddress(DEFAULT_IP_ADDRESS);
  }
}
