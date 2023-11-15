#include <Arduino.h>

// Нам надо отобразить 8 сегментные ИВ-9
// в 10 битные посадочные места.
// Для этого упакуем байты, перемежая их двумя пустыми битами. 
void populateIV9(byte* segBytes, byte* shiftBytes)
{
  shiftBytes[0] = segBytes[0];

  shiftBytes[1]  = segBytes[1] >> 2;
  shiftBytes[2]  = segBytes[1] << 6;

  shiftBytes[2] |= segBytes[2] >> 4;
  shiftBytes[3]  = segBytes[2] << 4;

  shiftBytes[3] |= segBytes[3] >> 6;
  shiftBytes[4]  = segBytes[3] << 2;
}