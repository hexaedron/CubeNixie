#pragma once

#define CLOCK      A1    //SH_CP 
#define DATA       A3    //DS   
#define LATCH      A2    //ST_CP 
#define PWM         2    // 
#define SW_DOTS     9

#define GMT_SECONDS_OFFSET (3 * 60 * 60) // Московское время +3 часа по умолчанию
#define DEFAULT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)0 
#define NIGHT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)1
#define GMT_OFFSET_HOURS_EEPROM_ADDRESS (uint16_t)2
#define GMT_OFFSET_MINUTES_EEPROM_ADDRESS (uint16_t)3
#define DOTS_NIGHT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)4
#define DOTS_DEFAULT_BRIGHTNESS_EEPROM_ADDRESS (uint16_t)5
#define DEFAULT_IP_ADDRESS_ADDRESS (uint16_t)6
#define DEFAULT_IP_ADDRESS 0x0101A8C0UL //192.168.1.1
#define DEFAULT_BRIGHTNESS 60 
#define NIGHT_BRIGHTNESS 75
#define DOTS_DAY_BRIGHTNESS 100
#define DOTS_NIGHT_BRIGHTNESS 75
#define DOTS_OFF 0
#define INIT_ADDR 512  // номер резервной ячейки
#define INIT_KEY 67     // ключ первого запуска. 0-254, на выбор


// Это на случай, если координаты не подгрузились
#define DEFAULT_SUNRISE_TIME (9 * 60)
#define DEFAULT_SUNSET_TIME (20 * 60)

//#define DEBUG_ENABLE // Для включения отладки раскомментировать
#define FAST_SHIFT_OUT
#define USE_SOFT_RTC
#define IV9_NIXIE
#define IV9_MUTATION "72301564" 
#define UNIX_2000_OFFSET 946684800UL

#ifdef DEBUG_ENABLE
  #define DEBUG(msg, x) Serial.print(msg); Serial.println(x)
  #define DEBUG_BIN(msg, x) Serial.print(msg); Serial.println((x), BIN)
  #define INFO(x) Serial.print(F("INFO: ")); Serial.println(x)
#else
  #define DEBUG(msg, x)
  #define DEBUG_BIN(msg, x)
  #define INFO(x)
#endif

typedef struct brightness
{
  uint8_t dots;   // Яркость точек в диапазоне 0-255
  uint8_t screen; // Яркость экрана в процентах
} brightness;

// Правильно заполняет массив с датой/временем
void makeDateTimeScreen(char* datetime, uint8_t hr, uint8_t min);

// Подготавливает и заполняет ИВ-9
void print_IV_9(void);
void populateIV9(byte* segBytes, byte* shiftBytes);

bool adjustTime(uint32_t GMTSecondsOffset);
void calculateBrightness();

uint8_t getDayBrightness(void);
void setDayBrightness(uint8_t dayBrightness);
uint8_t getNightBrightness(void);
void setNightBrightness(uint8_t nightBrightness);
void setGMTOffset(uint32_t offset);
uint32_t getGMTOffset(void);
uint8_t getDayDotsBrightness(void);
void setDayDotsBrightness(uint8_t dayBrightness);
uint8_t getNightDotsBrightness(void);
void setNightDotsBrightness(uint8_t nightBrightness);
uint32_t getIPAddress(void);
void setIPAddress(uint32_t address);
void EEPROMValuesInit(bool force = false);

uint16_t getMoscowSunrise(uint8_t month, uint8_t day);
uint16_t getMoscowSunset(uint8_t month, uint8_t day);