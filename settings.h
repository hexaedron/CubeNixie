#pragma once

#define CLOCK      A1   //SH_CP 
#define DATA       A3    //DS   
#define LATCH      A2    //ST_CP 
#define PWM         2   // 
#define SW_DOTS     9

#define GMT_SECONDS_OFFSET (3 * 60 * 60) // Московское время +3 часа по умолчанию
#define DEFAULT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)0 
#define NIGHT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)1
#define GMT_OFFSET_HOURS_EEPROM_ADDRESS (uint16_t)2
#define GMT_OFFSET_MINUTES_EEPROM_ADDRESS (uint16_t)3
#define DEFAULT_BRIGHTNESS 190 
#define NIGHT_BRIGHTNESS 40
#define DOTS_BRIGHTNESS 50
#define INIT_ADDR 1023  // номер резервной ячейки
#define INIT_KEY 66     // ключ первого запуска. 0-254, на выбор
#define SOFT_RTC_DELTA_T 1210 // Компенсация отставаний софт часов

// Это на случай, если координаты не подгрузились
#define DEFAULT_SUNRISE_TIME (9 * 60)
#define DEFAULT_SUNSET_TIME (20 * 60)

#define DEBUG_ENABLE // Для включения отладки раскомментировать
#define FAST_SHIFT_OUT
//#define NEED_GPS_SETUP
#define NUMBERS_ONLY // Для экрана
//#define USE_SOFT_SERIAL
#define SOFT_GPS_BAUD_RATE 9600
#define USE_SOFT_RTC
#define IV9_NIXIE
#define IV9_MUTATION "31267450"

#ifdef DEBUG_ENABLE
  #define DEBUG(msg, x) Serial.print(msg); Serial.println(x)
  #define DEBUG_BIN(msg, x) Serial.print(msg); Serial.println((x), BIN) 
#else
  #define DEBUG(msg, x)
  #define DEBUG_BIN(msg, x)
#endif

struct sunsetSunrise
{
  uint16_t set;
  uint16_t rise;
};

// Правильно заполняет массив с датой/временем
void makeDateTimeScreen(char* datetime, uint8_t hr, uint8_t min);

void populateIV9(byte* segBytes, byte* shiftBytes);

void adjustTime(uint32_t GMTSecondsOffset);
uint8_t calculateBrightness();

uint8_t getDayBrightness(void);
void setDayBrightness(uint8_t dayBrightness);
uint8_t getNightBrightness(void);
void setNightBrightness(uint8_t nightBrightness);
void setGMTOffset(uint32_t offset);
uint8_t getGMTOffset(void);
void EEPROMValuesInit(bool force = false);