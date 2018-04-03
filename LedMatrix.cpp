#include <SPI.h>
#include "LedMatrix.h"
#include "cp437font.h"

/**
 * Heavily influenced by the code and the blog posts from https://github.com/nickgammon/MAX7219_Dot_Matrix
 */
LedMatrix::LedMatrix(byte numberOfDevices, byte slaveSelectPin) {
    myNumberOfDevices = numberOfDevices;
    mySlaveSelectPin = slaveSelectPin;
    cols = new byte[numberOfDevices * 8];
}

/**
 *  numberOfDevices: how many modules are daisy changed togehter
 *  slaveSelectPin: which pin is controlling the CS/SS pin of the first module?
 */
void LedMatrix::init() {
    pinMode(mySlaveSelectPin, OUTPUT);
    
    SPI.begin ();
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV128);
    for(byte device = 0; device < myNumberOfDevices; device++) {
        sendByte (device, MAX7219_REG_SCANLIMIT, 7);   // show all 8 digits
        sendByte (device, MAX7219_REG_DECODEMODE, 0);  // using an led matrix (not digits)
        sendByte (device, MAX7219_REG_DISPLAYTEST, 0); // no display test
        sendByte (device, MAX7219_REG_INTENSITY, 0);   // character intensity: range: 0 to 15
        sendByte (device, MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
    }
}

void LedMatrix::sendByte (const byte device, const byte reg, const byte data) {
    int offset=device;
    int maxbytes=myNumberOfDevices;
    
    for(int i=0;i<maxbytes;i++) {
        spidata[i] = (byte)0;
        spiregister[i] = (byte)0;
    }
    // put our device data into the array
    spiregister[offset] = reg;
    spidata[offset] = data;
    // enable the line
    digitalWrite(mySlaveSelectPin,LOW);
    // now shift out the data
    for(int i=0;i<myNumberOfDevices;i++) {
        SPI.transfer (spiregister[i]);
        SPI.transfer (spidata[i]);
    }
    digitalWrite (mySlaveSelectPin, HIGH);
    
}

void LedMatrix::sendByte (const byte reg, const byte data) {
    for(byte device = 0; device < myNumberOfDevices; device++) {
        sendByte(device, reg, data);
    }
}

void LedMatrix::setIntensity(const byte intensity) {
    sendByte(MAX7219_REG_INTENSITY, intensity);
}

void LedMatrix::setTextAlignment(byte textAlignment) {
    myTextAlignment = textAlignment;
    calculateTextAlignmentOffset();
}

void LedMatrix::calculateTextAlignmentOffset() {
    switch(myTextAlignment) {
        case TEXT_ALIGN_LEFT:
            myTextAlignmentOffset = 0;
            break;
        case TEXT_ALIGN_LEFT_END:
            myTextAlignmentOffset = myNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT:
            myTextAlignmentOffset = myTextLength - myNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT_END:
            myTextAlignmentOffset = - myTextLength;
            break;
    }
    
}

void LedMatrix::clear() {
    for (byte col = 0; col < myNumberOfDevices * 8; col++) {
        cols[col] = 0;
    }
}

void LedMatrix::commit() {
  if (myDisplayOrientation) {
    for (byte dev = 0; dev < myNumberOfDevices; dev++) {
      byte m[8] = {0};
      for (byte col = 0; col < 8; col++) {
        byte b = cols[dev*8+col];
        for (byte bit = 0; bit < 8; bit++) {
          if (b & 1) m[bit] |= (128>>col);
          b >>= 1;
        }
      }
      for (byte col = 0; col < 8; col++)
        sendByte(dev, col + 1, m[col]);
    }
  }
  else {
    for (byte col = 0; col < myNumberOfDevices * 8; col++)
      sendByte(col / 8, col % 8 + 1, cols[col]);
  }
}

void LedMatrix::setText(String text) {
    myText = text;
    myTextOffset = 0;
    myTextLength = 0;
    for (int i = 0; i < myText.length(); i++) myTextLength += cp437_width[(byte)myText.charAt(i)];
    calculateTextAlignmentOffset();
}

void LedMatrix::setNextText(String nextText) {
    myNextText = nextText;
}

void LedMatrix::scrollTextRight() {
    myTextOffset = (myTextOffset + 1) % (myTextLength - 5);
}

void LedMatrix::scrollTextLeft() {
    myTextOffset = (myTextOffset - 1) % (myTextLength + myNumberOfDevices * 8);
    if (myTextOffset == 0 && myNextText.length() > 0) {
        myText = myNextText;
        myNextText = "";
        myTextLength = 0;
        for (int i = 0; i < myText.length(); i++) myTextLength += cp437_width[(byte)myText.charAt(i)];
        calculateTextAlignmentOffset();
    }
}

void LedMatrix::oscillateText() {
    int maxColumns = myTextLength;
    int maxDisplayColumns = myNumberOfDevices * 8;
    if (maxDisplayColumns > maxColumns) {
        return;
    }
    if (myTextOffset - maxDisplayColumns == -maxColumns) {
        increment = 1;
    }
    if (myTextOffset == 0) {
        increment = -1;
    }
    myTextOffset += increment;
}

void LedMatrix::setAlternateDisplayOrientation() {
    myDisplayOrientation = 1;
}

void LedMatrix::drawText() {
  boolean escaped = false;
  char letter;
  int  width;
  int  pos0 = myTextOffset + myTextAlignmentOffset;
  for (int i = 0; i < myText.length(); i++) {
    letter = myText.charAt(i);
    if (escaped)
    {
      switch ((byte)letter)
      {
        case 132: letter = (char)142;
                  break;
        case 150: letter = (char)153;
                  break;
        case 156: letter = (char)154;
                  break;
        case 164: letter = (char)132;
                  break;
        case 182: letter = (char)148;
                  break;
        case 188: letter = (char)129;
                  break;
        case 159: letter = (char)225;
                  break;
        default: break;
      }
      escaped = false;
    }
    else escaped = ((byte)letter == 195 || (byte)letter == 194);
    if (escaped) continue;
    width = pgm_read_byte (&cp437_width[(byte)letter]);
    for (byte col = 0; col < width; col++) {
      int position = pos0 + col;
      if (position >= 0 && position < myNumberOfDevices * 8 && col < 8) {
        setColumn(position, pgm_read_byte (&cp437_font [letter] [col]));
      }
    }
    pos0 += width;
  }
}

void LedMatrix::setColumn(int column, byte value) {
    if (column < 0 || column >= myNumberOfDevices * 8) {
        return;
    }
    cols[column] = value;
}

void LedMatrix::setPixel(byte x, byte y) {
    bitWrite(cols[x], y, true);
}
