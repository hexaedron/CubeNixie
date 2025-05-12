#include "settings.h"
#include "Timer3Pin2PWM.h"
#include <WDT.h>
#include <I2C_eeprom.h>

#include <TinyGPS++.h>

#include <PostNeoSWSerial.h>

#include "swRTC2000.h"
#include <RtcUtility.h>
#include <RtcDateTime.h>

#ifdef FAST_SHIFT_OUT
  #include <FastShiftOut.h>
  FastShiftOut FSO(DATA, CLOCK, LSBFIRST);
  #define shiftOut(DATA_PIN, CLOCK, ORDER, VALUE) FSO.write(VALUE)
#endif

#define SCREEN_DIGITS_NUM 4
#include "lib7SegmentScreenShifted.h"

#include "MoscowSetRise.h"
#include "simpleTimer.h"

// Устройства
swRTC2000 rtc; 
PostNeoSWSerial GPS_SoftSerial(RX_PIN, TX_PIN);
I2C_eeprom EEPROM(0b1010000, I2C_DEVICESIZE_24LC02); //Все адресные ножки 24LC02 подключаем к земле, это даёт нам адрес 0b1010000 или 0x50
TinyGPSPlus   ATGM332D;
TinyGPSCustom ATGM332D_year(ATGM332D,  "GNZDA", 4);
TinyGPSCustom ATGM332D_month(ATGM332D, "GNZDA", 3);
TinyGPSCustom ATGM332D_day(ATGM332D,   "GNZDA", 2);


// Буферы
char datetime[] = "0000";
byte shiftBytes[5] = {'\0'};
brightness Brightness = {50, 50};


#ifdef IV9_NIXIE
  byte* segBytes;
  sevenSegmentScreenShifted IV9Screen(LATCH, DATA, CLOCK, COMMON_CATHODE);
#endif

void setup() 
{
  // Сразу поставим небольшую яркость, чтобы не пожечь лампы от 5В
  initTimer3Pin2PWM_32_2000(95, 75);
  wdt_enable(WTO_16S); // Ставим вотчдог. пришлось допилить либу Ethernet, воткнув в неё wdt_reset() в блокирующих местах

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);
    wdt_reset();
    INFO("Starting");
  #endif

  INFO("EEPROM");
    wdt_reset();
    EEPROMValuesInit();
  INFO("EEPROM ok!");

  
  pinMode(DATA,    OUTPUT);
  pinMode(LATCH,   OUTPUT);
  pinMode(CLOCK,   OUTPUT);
  pinMode(SW_DOTS, OUTPUT);
  pinMode(TX_PIN,     OUTPUT);
  pinMode(RX_PIN,      INPUT);

  wdt_reset();

  INFO("Start GPS");
  // Получим адрес по DHCP. 
  datetime[0] = 'G';
  datetime[1] = 'P';
  datetime[2] = 'S';
  datetime[3] = '\0';
  print_IV_9();

  GPS_SoftSerial.begin(SOFT_GPS_BAUD_RATE);
  rtc.setDeltaT(SOFT_RTC_DELTA_T);

  while(GPS_SoftSerial.available() > 0)
  {
      ATGM332D.encode(GPS_SoftSerial.read());
      wdt_reset();
  }

  while(!adjustTime(getGMTOffset()))
  {
    while(GPS_SoftSerial.available() > 0)
    {
      ATGM332D.encode(GPS_SoftSerial.read());
      wdt_reset();
    }
    DEBUG("year=", ATGM332D.date.year());
    DEBUG("time=", ATGM332D.time.value());
  }

  while(!adjustTime(getGMTOffset()))
  {
    wdt_reset();
    DEBUG("year=", ATGM332D.date.year());
    DEBUG("time=", ATGM332D.time.value());
  }
  INFO("GPS ok!");

  wdt_reset();
  calculateBrightness();
  setTimer3Pin2PWMDuty(Brightness.screen);
  wdt_reset();
}

void loop() 
{
  uint8_t hour = rtc.getHours();
  uint8_t minute = rtc.getMinutes();
  uint8_t second = rtc.getSeconds();
  bool minRefreshFlag = true;
  bool dotRefreshFlag = true;
  Timer16 clockTimer(500);

  for(;;)
  {
    wdt_reset();

    // Время с датчика надо брать постоянно, чтобы не переполнился буфер
    while(GPS_SoftSerial.available() > 0)
      ATGM332D.encode(GPS_SoftSerial.read());
    
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

    //Каждые 2 минуты подводим часы
    if(minute % (2)) 
    {
      if(minRefreshFlag)
      {
        adjustTime(getGMTOffset());
        minRefreshFlag = false;
        calculateBrightness();
        setTimer3Pin2PWMDuty(Brightness.screen);
      }
    }
    else
    {
      minRefreshFlag = true;
    }
    wdt_reset();
  }
}

// *******************************************************************

// Подготавливает и заполняет ИВ-9
void print_IV_9()
{
  wdt_reset();

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

// Подводит время по GPS
bool adjustTime(uint32_t GMTSecondsOffset)
{
  wdt_reset();

  while(GPS_SoftSerial.available() > 0)
  {
      ATGM332D.encode(GPS_SoftSerial.read());
      wdt_reset();
  }
  
  if(GPS_TIME_IS_VALID())
  {
    RtcDateTime dt
    (
      //atoi(ATGM332D_year.value()), 
      //atoi(ATGM332D_month.value()), 
      //atoi(ATGM332D_day.value()), 
      ATGM332D.date.year(), 
      ATGM332D.date.month(),
      ATGM332D.date.day(),
      ATGM332D.time.hour(), 
      ATGM332D.time.minute(),
      ATGM332D.time.second() 
    );
    dt += GMTSecondsOffset;
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
  uint32_t    timeSet  = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0, 0).TotalSeconds() + 
                          (uint32_t)getMoscowSunset(timeNow.Month(), timeNow.Day()) * 60UL + getGMTOffset(); 
  uint32_t    timeRise = RtcDateTime(timeNow.Year(), timeNow.Month(), timeNow.Day(), 0, 0, 0).TotalSeconds() + 
                          (uint32_t)getMoscowSunrise(timeNow.Month(), timeNow.Day()) * 60UL + getGMTOffset(); 
  
  wdt_reset();

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