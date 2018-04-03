#include <SPI.h>
#include "LedMatrix.h"
 
#define NUMBER_OF_DEVICES 6
#define CS_PIN 15
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);
 
void setup()
{
  String text = "Zwölf laxe Typen qualmen verdächtig süße Objekte. ÜBEN VON XYLOPHON UND QUERFLÖTE IST JA ZWECKMÄSSIG. Typograf Jakob zürnt schweißgequält vom öden Text. 0123456789  ";
  ledMatrix.init();
  ledMatrix.setAlternateDisplayOrientation(); // devices 90° right
  ledMatrix.setIntensity(4); // range is 0-15

  for (byte b = 1; b < 255; b++)
  {
    if (b == 195 || b == 194) text += (char)b;
    text += (char)b;
  }
  ledMatrix.setText(text);
}
 
void loop()
{
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(5);
}

