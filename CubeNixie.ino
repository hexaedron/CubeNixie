#include "settings.h"
#include "Timer3Pin2PWM.h"
#include <WDT.h>

#include "swRTC2000.h"
#include <RtcUtility.h>
#include <RtcDateTime.h>

#ifdef FAST_SHIFT_OUT
  #include <FastShiftOut.h>
  #define shiftOut(DATA_PIN, CLOCK, ORDER, VALUE) FSO.write(VALUE)
  FastShiftOut FSO(DATA, CLOCK, LSBFIRST);
#endif

#define SCREEN_DIGITS_NUM 4
#include "lib7SegmentScreenShifted.h"

swRTC2000 rtc;
char datetime[] = "0000";
byte shiftBytes[5] = {'\0'};

#ifdef IV9_NIXIE
  char* segBytes;
  sevenSegmentScreenShifted IV9Screen(LATCH, DATA, CLOCK, COMMON_CATHODE);
#endif

void setup() 
{
  initTimer3Pin2PWM_32_2000(95, 75);
  wdt_enable(WTO_1S);

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);
  #endif
  
  pinMode(DATA, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);

  rtc.stopRTC();
    rtc.setDate(17, 12, 2023);
    rtc.setTime(16, 20, 0);
  rtc.startRTC();
}

void loop() 
{
  
  for(;;)
  {
    IV9Screen.renderBytes(datetime);
    IV9Screen.mutate(IV9_MUTATION);
    segBytes = IV9Screen.getRawBytes();
    populateIV9(segBytes, shiftBytes);
    
    //Serial.print((byte)shiftBytes[0], BIN);
    //Serial.print(' ');
    //Serial.print((byte)shiftBytes[1], BIN);
    //Serial.print(' ');
    //Serial.print((byte)shiftBytes[2], BIN);
    //Serial.print(' ');
    //Serial.print((byte)shiftBytes[3], BIN);
    //Serial.print(' ');
    //Serial.println((byte)shiftBytes[4], BIN);

    delay(500);
    digitalWrite(LATCH, LOW);
    for (int i = 0; i < 5; i++)
    {
      shiftOut(DATA, CLOCK, LSBFIRST, shiftBytes);
    }
    digitalWrite(LATCH, HIGH);
    
    wdt_reset();
  }

}
