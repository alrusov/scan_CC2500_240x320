/*************************************************************************************************
 * Portable scanner based on CC2500 single chip or related CC2500 module
 * Written by Valeriy Yatsenkov (aka Rover) http://www.fankraft.ru
 * Rewrited and enhanced by Alexey Rusov
 * Based on oriiginal Ver.1.2 March,14,2015
 * You may freely copy and redistribute this software if no fee is charged for use,
 * copying or distribution.  Such redistributions must retain the above copyright
 * notice.
 **************************************************************************************************
 * Precautions:
 * - Do appropriate current and voltage conversion between your microcontroller and CC2500 module.
 * - High voltage or High current may damage your CC2500 Module or Display Module!
 **************************************************************************************************
 * scanner for
 * base frequency 2400.009949 MHz (channel 0)
 * end frequency  2483.128540 MHz (channel 205)
 * channel spacing 405.456543 kHz
 **************************************************************************************************
 * Graphics implementation based on Adafruit_GFX core library
 * https://github.com/adafruit/Adafruit-GFX-Library/archive/master.zip
 *
 * carefully check what display chip used on your display module!!!
 * library for ILI9341 is Adafruit_ILI9341
 * https://github.com/adafruit/Adafruit_ILI9341/archive/master.zip
 * library for ILI9341 is Adafruit_ILI9341
 * https://github.com/adafruit/Adafruit-ILI9341-Library/archive/master.zip
 * library for Samsung S6D02A1 is Adafruit_QDTech
 * https://github.com/zigwart/Adafruit_QDTech
 *
 * Thanks Adafruit for free libraries. You can purchase display modules at http://www.adafruit.com
 */

//-----------------------------------------------------------------------------------------------------------------------------------------//

#include <Arduino.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library for ILI9341 display chip
#include <SPI.h>
#include "cc2500_REG.h"       // CC2500 registers description

//#define DEBUG
#include <Arduino.h>

// https://github.com/alrusov/arduino._mylibs
#include "_mylibs.h"

#define TFT_RST    7      // display Reset (0 if use Arduino reset)
#define TFT_DC     8      // display Data/Command control line
#define TFT_CS     9      // display Select

#define SCAN_CS   10      // scanner Select

#define CHAN_COUNT      206 // max number of channel for spacing 405.456543MHz
#define SAMPLES_COUNT   100 // qty of samples in each iteration (1...100) to found a max RSSI value

#define TFT_ROTATION    0

#define MAX_VAL   128

#define TFT_BLACK   ILI9341_BLACK
#define TFT_WHITE   ILI9341_WHITE
#define TFT_RED     ILI9341_RED
#define TFT_GREEN   ILI9341_GREEN
#define TFT_BLUE    ILI9341_BLUE

#define TEXT_SIZE   2   // *8
#define INFO_H      (3 + TEXT_SIZE*8 + 3)

#define FRAME_X   16
#define FRAME_Y   5
#define FRAME_W   (1 + CHAN_COUNT + 1)
#define FRAME_H   (1 + INFO_H + MAX_VAL + 1)

#define GRAPH_Y (FRAME_Y + 1 + INFO_H)
#define GRAPH_H MAX_VAL

#define INFO_X (FRAME_X + 1 + 46)   // X position of frequency data
#define INFO_Y (FRAME_Y + 1 + 3)    // Y position of frequency data

//-----------------------------------------------------------------------------------------------------------------------------------------//

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST); // use hardware SPI as more fast

byte  Caliber[CHAN_COUNT];
int   MarkerAbs = -1;
int   MarkerPos = -1;

//-----------------------------------------------------------------------------------------------------------------------------------------//

void CC2500_Ready() {
  while(digitalRead(MISO) == HIGH);
}


byte CC2500_Read(byte addr) {
  digitalWrite(SCAN_CS, LOW);
  CC2500_Ready();

  SPI.transfer(addr);
  byte value = SPI.transfer(0);
  digitalWrite(SCAN_CS, HIGH);
  
  return value;
}


void CC2500_Write(byte addr, byte value) {
  digitalWrite(SCAN_CS, LOW);
  CC2500_Ready();
  
  SPI.transfer(addr);
  SPI.transfer(value);
  digitalWrite(SCAN_CS, HIGH);
}


void CC2500_init() {
  CC2500_Write(SRES     , 0x3D);  // Software reset for CC2500
  CC2500_Write(FSCTRL1  , 0x0F);  // Frequency Synthesizer Control (0x0F)
  CC2500_Write(PKTCTRL0 , 0x12);  // Packet Automation Control (0x12)
  CC2500_Write(FREQ2    , 0x5C);  // Frequency control word, high byte
  CC2500_Write(FREQ1    , 0x4E);  // Frequency control word, middle byte
  CC2500_Write(FREQ0    , 0xDE);  // Frequency control word, low byte
  CC2500_Write(MDMCFG4  , 0x0D);  // Modem Configuration
  CC2500_Write(MDMCFG3  , 0x3B);  // Modem Configuration (0x3B)
  CC2500_Write(MDMCFG2  , 0x00);  // Modem Configuration 0x30 - OOK modulation, 0x00 - FSK modulation (better sensitivity)
  CC2500_Write(MDMCFG1  , 0x23);  // Modem Configuration
  CC2500_Write(MDMCFG0  , 0xFF);  // Modem Configuration (0xFF)
  CC2500_Write(MCSM1    , 0x0F);  // Always stay in RX mode
  CC2500_Write(MCSM0    , 0x04);  // Main Radio Control State Machine Configuration (0x04)
  CC2500_Write(FOCCFG   , 0x15);  // Frequency Offset Compensation configuration
  CC2500_Write(AGCCTRL2 , 0x83);  // AGC Control (0x83)
  CC2500_Write(AGCCTRL1 , 0x00);  // AGC Control
  CC2500_Write(AGCCTRL0 , 0x91);  // AGC Control
  CC2500_Write(FSCAL3   , 0xEA);  // Frequency Synthesizer Calibration
  CC2500_Write(FSCAL2   , 0x0A);  // Frequency Synthesizer Calibration
  CC2500_Write(FSCAL1   , 0x00);  // Frequency Synthesizer Calibration
  CC2500_Write(FSCAL0   , 0x11);  // Frequency Synthesizer Calibration
}

//-----------------------------------------------------------------------------------------------------------------------------------------//

void setup(void) {
  debugInit();

  pinMode(TFT_CS, OUTPUT);
  pinMode(SCAN_CS, OUTPUT);
//  pinMode(SS, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SCAN_CS, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // max possible SPI speed, 1/2 F_CLOCK

  tft.begin();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK);  // clear display to black

  tft.setCursor(0, 5);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE);
  tft.print("Calibration...");

  CC2500_init();  // initialize CC2500 registers

  // collect and save calibration data for each of channel
  tft.drawRect(FRAME_X, 50, FRAME_W, 22, TFT_WHITE);

  for (int i = 0; i < CHAN_COUNT; i++) {
    CC2500_Write(CHANNR , i);                 // set channel
    CC2500_Write(SIDLE  , 0x3D);              // idle mode
    CC2500_Write(SCAL   , 0x3D);              // start manual calibration
    delayMicroseconds(800);                   // wait for calibration
    byte data = CC2500_Read(FSCAL1 + 0x80);   // read calibration value
    Caliber[i] = data;                        // and store it
    tft.drawFastVLine(FRAME_X + 1 + i, 50 + 1, 20, TFT_GREEN);  //progress bar on screen
  }

  CC2500_Write(CHANNR , 0x00);  // set channel
  CC2500_Write(SFSTXON, 0x3D);  // calibrate and wait
  delayMicroseconds(800);       // settling time, refer to datasheet
  CC2500_Write(SRX, 0x3D);      // enable rx

  tft.fillScreen(TFT_BLACK);
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, TFT_WHITE);
}

//-----------------------------------------------------------------------------------------------------------------------------------------//

void DrawMarker() {
  int oldPos = MarkerPos;
  int newAbs = analogRead(A7);
  
  if ((MarkerPos < 0) || (abs(MarkerAbs - newAbs) > 4)) {
    MarkerAbs = newAbs;
    MarkerPos = (int)(newAbs / 5. + 0.5);
  }

  if (MarkerPos != oldPos) {
    tft.fillRect(INFO_X, INFO_Y, FRAME_X + FRAME_W - INFO_X - 2, TEXT_SIZE*8, TFT_BLACK);
    tft.setCursor(INFO_X, INFO_Y);

    double chan = 0.405456543 * MarkerPos + 2400.009949;
    tft.print(chan, 4);

    tft.print("/");

    char s[4];
    sprintf(s, "%03d", MarkerPos);
    tft.print(s);

    if(oldPos >= 0) {
      tft.drawFastVLine(FRAME_X + 1 + oldPos, GRAPH_Y, GRAPH_H, TFT_BLACK);
    }
    tft.drawFastVLine(FRAME_X + 1 + MarkerPos, GRAPH_Y, GRAPH_H, TFT_RED);
  }
}


void loop() {
  for (int i = 0; i < CHAN_COUNT; i++) {
    CC2500_Write(CHANNR, i);            // set channel
    CC2500_Write(FSCAL1, Caliber[i]);   // restore calibration value for this channel
    delayMicroseconds(300);             // settling time, refer to datasheet

    int maxDbm = -120;

    for (int j = 0; j < SAMPLES_COUNT; j++) { // collect samples for max value
      byte data = CC2500_Read(REG_RSSI);

      // convert RSSI data from 2's complement to signed decimal
      int dbm = data >= 128?
                 (data - 256) / 2 - 70:
                 data / 2 - 70;
      if (dbm > maxDbm) {
        maxDbm = dbm; // keep maximum
      }
    }

//#define RSSI_OFFSET     95  // offset for displayed data
    maxDbm += 95;
    
    if(maxDbm > MAX_VAL) {
      maxDbm = MAX_VAL;
    } else if (maxDbm < 0) {
      maxDbm = 1;
    }

    if (i != MarkerPos) { // if channel position not equal to marker position
      tft.drawFastVLine(FRAME_X + 1 + i, FRAME_Y + 22                  , MAX_VAL - maxDbm, TFT_BLACK);  // draw spectrum lines
      tft.drawFastVLine(FRAME_X + 1 + i, FRAME_Y + FRAME_H - 1 - maxDbm, maxDbm      , TFT_GREEN);
    }
  }
  
  DrawMarker();
}

//-----------------------------------------------------------------------------------------------------------------------------------------//
