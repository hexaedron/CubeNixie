#include "settings.h"
#include "Timer3Pin2PWM.h"
#include <WDT.h>
#include <I2C_eeprom.h>

#include "NTPClient.h"
#define W5500_ETHERNET_SHIELD
#include "SPI.h"
#include "Ethernet.h"
#include "EthernetUdp.h"

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

// Устройства
swRTC2000 rtc; 
//EthernetClient client;
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, IPAddress(192,168,1,1));
I2C_eeprom EEPROM(0b1010000, I2C_DEVICESIZE_24LC02); //Все адресные ножки 24LC02 подключаем к земле, это даёт нам адрес 0b1010000 или 0x50

// Буферы
char datetime[] = "0000";
byte shiftBytes[5] = {'\0'};
brightness Brightness = {0, 0};
byte mac[6] = {0x66, 0xAA, (uint8_t)&GUID0, (uint8_t)&GUID1, (uint8_t)&GUID2, (uint8_t)&GUID3}; // MAC-адрес будет формироваться уникальный для каждого чипа


#ifdef IV9_NIXIE
  byte* segBytes;
  sevenSegmentScreenShifted IV9Screen(LATCH, DATA, CLOCK, COMMON_CATHODE);
#endif

void setup() 
{
  // Сразу поставим небольшую яркость, чтобы не пожечь лампы от 5В
  initTimer3Pin2PWM_32_2000(95, 50);
  wdt_enable(WTO_1S); // Ставим вотчдог. пришлось допилить либу Ethernet, воткнув в неё wdt_reset() в блокирующих местах

  EEPROMValuesInit();

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);
  #endif
  
  pinMode(DATA,    OUTPUT);
  pinMode(LATCH,   OUTPUT);
  pinMode(CLOCK,   OUTPUT);
  pinMode(SW_DOTS, OUTPUT);

  INFO("Start DHCP");
  // Получим адрес по DHCP. Пока получаем, рисуем анимацию
  while (Ethernet.begin(mac) == 0) 
  {
    wdt_reset();
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
    
    wdt_reset();
    delay(500);
    wdt_reset();
  }
  INFO("DHCP ok!");

  INFO("Start NTP");
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
    
    wdt_reset();
    delay(500);
    wdt_reset();
  }
  INFO("NTP ok!");

  calculateBrightness();
  setTimer3Pin2PWMDuty(Brightness.screen);
}

void loop() 
{
  uint8_t hour = rtc.getHours();
  uint8_t minute = rtc.getMinutes();
  uint8_t second = rtc.getSeconds();
  bool minRefreshFlag = true;
  bool dotRefreshFlag = true;
  Timer16 clockTimer(500);
  
  // Это время в минутах, прибавляемое к периоду обновления NTP, 
  // чтобы девайсы не дёргали NTP сервер одновременно.
  uint8_t minAdd = (uint8_t)map((uint32_t)&GUID0, 0, 0xFFFFFFFF, 1, 10);

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

      DEBUG("Timestamp2000     = ", rtc.getTimestamp2000());
      DEBUG("Timestamp         = ", rtc.getTimestamp());
      DEBUG("Screen Brightness = ", Brightness.screen);
      DEBUG("Dots Brightness   = ", Brightness.dots);
    }

    if(minute % (3 + minAdd)) 
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

// Подводит время по NTP
bool adjustTime(uint32_t GMTSecondsOffset)
{
  if(timeClient.update())
  {
    timeClient.setTimeOffset(GMTSecondsOffset);
    RtcDateTime dt(timeClient.getEpochTime() - UNIX_2000_OFFSET);
    rtc.stopRTC();
      rtc.setDate(dt.Day(), dt.Month(), dt.Year());
      rtc.setTime(dt.Hour(), dt.Minute(), dt.Second());
    rtc.startRTC();
    return true;
  }

  return false;
}

// вычисляет яркость точек и самого экрана
void calculateBrightness()
{
  RtcDateTime timeNow(rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  uint32_t    timeSet  = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0 ,0).TotalSeconds() + (uint32_t)getMoscowSunset(timeNow.Month(), timeNow.Day()) * 60; 
  uint32_t    timeRise = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0 ,0).TotalSeconds() + (uint32_t)getMoscowSunrise(timeNow.Month(), timeNow.Day()) * 60; 
  
  if((timeNow.TotalSeconds() > timeRise) && (timeNow.TotalSeconds() < timeSet)) // День, поскольку мы между закатом и рассветом
  {
    DEBUG("calculateBrightness(): ", "It's a day!");
    Brightness.screen = getDayBrightness();
    Brightness.dots   = getDayDotsBrightness();
  }
  else
  {
    DEBUG("calculateBrightness(): ", "It's a night!");
    Brightness.screen = getNightBrightness();
    Brightness.dots   = getNightDotsBrightness();
  }
}