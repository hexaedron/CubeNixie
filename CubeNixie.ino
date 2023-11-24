#include "settings.h"
#include "Timer3Pin2PWM.h"
#include <WDT.h>
#include <I2C_eeprom.h>

#include <NTPClient.h>
#define W5500_ETHERNET_SHIELD
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include "swRTC2000.h"
#include <RtcUtility.h>
#include <RtcDateTime.h>

#ifdef FAST_SHIFT_OUT
  #include <FastShiftOut.h>
  FastShiftOut FSO(DATA, CLOCK, LSBFIRST);
  #define shiftOut(DATA_PIN, CLOCK, ORDER, VALUE) FSO.write(VALUE)
#endif

#define SCREEN_DIGITS_NUM 4
#define NUMBERS_ONLY
#include "lib7SegmentScreenShifted.h"

#include "MoscowSetRise.h"
#include "simpleTimer.h"

swRTC2000 rtc;
char datetime[] = "0000";
byte shiftBytes[5] = {'\0'};
I2C_eeprom EEPROM(0b1010000, I2C_DEVICESIZE_24LC02); //Все адресные ножки 24LC02 подключаем к земле, это даёт нам адрес 0b1010000 или 0x50
brightness Brightness = {0, 0};
byte mac[6] = 
{
  0x66, 0xAA, GUID0, GUID1, GUID2, GUID3
}; // MAC-адрес будет формироваться уникальный для каждого чипа
EthernetClient client;
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, IPAddress(192,168,1,1));

#ifdef IV9_NIXIE
  byte* segBytes;
  sevenSegmentScreenShifted IV9Screen(LATCH, DATA, CLOCK, COMMON_CATHODE);
#endif

void setup() 
{
  initTimer3Pin2PWM_32_2000(95, 75);
  wdt_enable(WTO_1S);

  EEPROMValuesInit();

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);
  #endif
  
  pinMode(DATA,    OUTPUT);
  pinMode(LATCH,   OUTPUT);
  pinMode(CLOCK,   OUTPUT);
  pinMode(SW_DOTS, OUTPUT);

  // Получим адрес по DHCP
  if (Ethernet.begin(mac) == 0) 
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

  // В начале обновляем время до упора. Пока идёт обновление, показываем растущую линию из точек.
  while(!adjustTime(getGMTOffset()))
  {
    uint8_t i = 0;
    uint8_t j = 1;

    for(i = 0; i < j; i++)
    {
      datetime[i] = '.';
    }

    j++;
    if(j > 4) j = 1;
    datetime[i] = '\0';
    print_IV_9();
    
    delay(500);
  }
}

void loop() 
{
  uint8_t hour = rtc.getHours();
  uint8_t minute = rtc.getMinutes();
  uint8_t second = rtc.getSeconds();
  bool minRefreshFlag = true;
  bool dotRefreshFlag = true;
  Timer16 clockTimer(500);
  
  calculateBrightness();

  for(;;)
  {
    if(clockTimer.ready())
    {
      hour   = rtc.getHours();
      minute = rtc.getMinutes();
      second = rtc.getSeconds();
    }

    if(((second % 2) && dotRefreshFlag))
    {
      analogWrite(SW_DOTS, Brightness.dots);

      makeDateTimeScreen(datetime, hour, minute);
      print_IV_9();
      dotRefreshFlag = !dotRefreshFlag;
    }
    else if((!(second % 2)) && !dotRefreshFlag)
    {
      analogWrite(SW_DOTS, DOTS_OFF);

      makeDateTimeScreen(datetime, hour, minute);
      print_IV_9();
      dotRefreshFlag = !dotRefreshFlag;
    }

    if(minute % 5 ) 
    {
      if(minRefreshFlag)
      {
        adjustTime(getGMTOffset());
        minRefreshFlag = false;
        calculateBrightness();
        setTimer3Pin2PWMDuty(Brightness.screen);
      }
    }
    wdt_reset();
  }
}

// *******************************************************************

// Подготавливает и заполняет ИВ-9
void print_IV_9()
{
  IV9Screen.renderBytes(datetime);
  IV9Screen.mutate(IV9_MUTATION);
  segBytes = IV9Screen.getRawBytes();
  populateIV9(segBytes, shiftBytes);

  digitalWrite(LATCH, LOW);
  for (int8_t i = 4; i >= 0; i--)
  {
    shiftOut(DATA, CLOCK, LSBFIRST, shiftBytes[i]);
  }
  digitalWrite(LATCH, HIGH);
}

bool adjustTime(uint32_t GMTSecondsOffset)
{
  if(timeClient.update())
  {
    timeClient.setTimeOffset(GMTSecondsOffset);
    RtcDateTime dt(timeClient.getEpochTime() + UNIX_2000_OFFSET);
    rtc.stopRTC();
      rtc.setDate(dt.Day(), dt.Month(), dt.Year());
      rtc.setTime(dt.Hour(), dt.Minute(), dt.Second());
    rtc.startRTC();
    return true;
  }

  return false;

}


void calculateBrightness()
{
  
  RtcDateTime timeNow(rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  uint32_t    timeSet  = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0 ,0).TotalSeconds() + (uint32_t)getMoscowSunset(timeNow.Month(), timeNow.Day()) * 60; 
  uint32_t    timeRise = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0 ,0).TotalSeconds() + (uint32_t)getMoscowSunrise(timeNow.Month(), timeNow.Day()) * 60; 
  
  if((timeNow.TotalSeconds() > timeRise) && (timeNow.TotalSeconds() < timeSet)) // День, поскольку мы между закатом и рассветом
  {
    DEBUG("calculateBrightness(): ", "It's a day!");
    Brightness.screen = getDayBrightness();
    Brightness.dots   = DOTS_DAY_BRIGHTNESS;
  }
  else
  {
    DEBUG("calculateBrightness(): ", "It's a night!");
    Brightness.screen = getNightBrightness();
    Brightness.dots   = DOTS_NIGHT_BRIGHTNESS;
  }
}