#include <Arduino.h>

// Нам надо отобразить 8 сегментные ИВ-9
// в 10 битные посадочные места.
// Для этого упакуем байты, перемежая их двумя пустыми битами. 
void populateIV9(byte* segBytes, byte* shiftBytes)
{
  // Нулевой байт segBytes копируем как есть
  shiftBytes[0] = segBytes[0];

  // Обработка 1 байта segBytes
  shiftBytes[1]  = segBytes[1] >> 2;
  shiftBytes[2]  = segBytes[1] << 6;

  // Обработка 2 байта segBytes
  // Фиксим ошибку в плате - инвертированные ножки у U7
  for (uint8_t i = 0; i <= 4; i++)
  {
    bitWrite(shiftBytes[2], i, bitRead(segBytes[2], 7 - i));
  }
  shiftBytes[3]  = segBytes[2] << 4;

  // Обработка 3 байта segBytes
  shiftBytes[3] |= segBytes[3] >> 6;
  shiftBytes[4]  = segBytes[3] << 2;
}