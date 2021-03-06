/*
  TSL1402R.cpp - Library for reading TSL1402R linear photodiode array
  Created by Douglas Mayhew, September 31, 2016.
  Released into the public domain.
*/

#include "Arduino.h"
#include "ADC.h" 	// Teensy ADC library
#include "TSL1402R.h"

//========================================================================================
// Default Pins for the TSL1402R sensor.
int _CLKpin = 24; // <-- Arduino pin delivering the clock pulses to pin 3(CLK) of the TSL1402R
int _SIpin = 25; // <-- Arduino pin delivering the SI (serial-input) pulse to pin 2 of the TSL1402R
int _Apin1 = 14; // <-- Arduino pin connected to pin 4 (analog output 1) of the TSL1402R
int _Apin2 = 39; // <-- Arduino pin connected to pin 8 (analog output 2) (parallel mode only)
//========================================================================================

uint16_t _sample1 = 0; // temporary place to hold A0 ADC value
uint16_t _sample2 = 0; // temporary place to hold A1 ADC value
uint16_t ExposureMicroseconds =500;
extern ADC *adc;
extern ADC::Sync_result ADCresult;

TSL1402R::TSL1402R(int CLKpin, int SIpin, int Apin1, int Apin2)
{
  _CLKpin = CLKpin;
  _SIpin = SIpin;
  _Apin1 = Apin1;
  _Apin2 = Apin2;
  ExposureMicroseconds = 500;
  initTSL1402R();
}

void TSL1402R::initTSL1402R() {
  // Initialize Arduino pins
  pinMode(_CLKpin, OUTPUT);
  pinMode(_SIpin, OUTPUT);
  
  pinMode(_Apin1, INPUT);
  pinMode(_Apin2, INPUT);
  
  // Set IO pins low: (there may be a built-in method for this in
  // Ardunio library for ESP8266)
    digitalWrite(_SIpin, LOW);
    digitalWrite(_CLKpin, LOW);

  // Clock out any existing SI pulse through the ccd register:
  for (int i = 0; i < 258; i++) {
    digitalWrite(_CLKpin, HIGH);
    digitalWrite(_CLKpin, LOW);
  }

  // Create a new SI pulse and clock out that same SI pulse through the sensor register:
  digitalWrite(_SIpin, HIGH);
  digitalWrite(_CLKpin, HIGH);
  digitalWrite(_SIpin, LOW);
  digitalWrite(_CLKpin, LOW);

  for (int i = 0; i < 258; i++) {
    digitalWrite(_CLKpin, HIGH);
    digitalWrite(_CLKpin, LOW);
  }
}

void TSL1402R::read(uint8_t * data, uint32_t len)
{
  // Create a new SI pulse:
  digitalWrite(_SIpin, HIGH);
  digitalWrite(_CLKpin, HIGH);
  digitalWrite(_SIpin, LOW);
  digitalWrite(_CLKpin, LOW);

  // A new measuring cycle is starting once 18 clock pulses have passed. At
  // that time, the photodiodes are once again active. We clock out the SI pulse through
  // the 256 bit register in order to be ready to halt the ongoing measurement at our will
  // (by clocking in a new SI pulse upon the next loop):

  for (int i = 0; i < 256; i++) {// 0 to 255 = 1 to 256 clock pulses(pixels)
    //      if (i == 18)
    //      {
    //        // Now the photodiodes goes active..
    //        // An external trigger can be placed here
    //      }
    digitalWrite(_CLKpin, HIGH);
    digitalWrite(_CLKpin, LOW);
  }

  // The integration time of the current program / measurement cycle is ~3ms (On Arduino 16 Mhz). 
  //If a larger timeb of integration is wanted, uncomment the next line:
  delayMicroseconds(ExposureMicroseconds); // <-- Add ExposureMicroseconds microseconds integration time

  // Stop the ongoing integration of light quanta from each photodiode by clocking in a new 
  //SI pulse into the sensors register:

  digitalWrite(_SIpin, HIGH);
  digitalWrite(_CLKpin, HIGH);
  digitalWrite(_SIpin, LOW);
  digitalWrite(_CLKpin, LOW);
  
  // Next, read all 256 pixels, 2 at a time (parallel mode). 
  // Teensy ADC library goes one step further, reading both pins at the same moment, rather than
  // one after the other, so twice as fast almost, at least in theory.
  // Store the result in the array. Each clock pulse causes a new pair of pixels to expose its 
  // value on the two outputs:
  
  for (int i = 0; i < 128; i++) { // 0 to 255 = 1 to 256 clock pulses(pixels)
    ADCresult = adc->analogSynchronizedRead(_Apin1, _Apin2);

    digitalWrite(_CLKpin, HIGH); // clocking in 257th clock pulse to sensor
    digitalWrite(_CLKpin, LOW);

    // we write two bytes per pixel to data[]
    _sample1 = ADCresult.result_adc0<<2;
    _sample2 = ADCresult.result_adc1<<2;

    data[i<<1] = (_sample1 >> 8) & 0xFF;           // AOpin1 HighByte 
    data[(i<<1)+1] = _sample1 & 0xFF;              // AOpin1 LowByte

    data[(i+128)<<1] = (_sample2 >> 8) & 0xFF;     // AOpin2 HighByte
    data[((i+128)<<1)+1] = _sample2 & 0xFF;        // AOpin2 LowByte
  }
  // the sensor data is now in the array that was passed into this function.
}