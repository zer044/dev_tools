///***************************************************************
///@file gpPins.h
///@brief Pin definitions for the Greyparrot Controller Board
///***************************************************************

#ifndef GPPINS_H_
#define GPPINS_H_

#include <Arduino.h>

#ifdef __AVR__
    //#define CONTROLLINO_MINI
#endif

#ifdef CONTROLLINO_MINI
    // connections to the camera and the lights
    #define PIN_ERROR_LED 9							// output PIN for the red error LED, Controllino Pin D5
    #define PIN_LIGHTING_PNP PIN_A4					// output PIN to be connect to the lights STROBE_IN, Controllino Pin D6
    #define PIN_CAMERA_TRIGGER_IN PIN_A5			// output PIN to be connected to the camera's TRIGGER_IN(+) PIN, Controllino Pin D7
    #define PIN_CAMERA_STROBE_OUT 3					// input interrupt PIN IN1 to be connected to the camera's STROBE_OUT indicating the exposure time.
    #define PIN_FAN 7								// output PIN for fan, Arduino PIN_D4, Controllino D3

    #define PIN_DAISY_OUT0 10						// Daisy chain output pin
    #define PIN_DAISY_OUT1 11						// Daisy chain output pin
    #define PIN_DAISY_OUT2 12						// Daisy chain output pin
    #define PIN_DAISY_IN0 2							// interruptible PIN, Daisy Chain Input Pin
    #define PIN_DAISY_IN1 PIN_A0					// Daisy Chain Input Pin
    #define PIN_DAISY_IN2 PIN_A1					// Daisy Chain Input Pin

#else
    // connections to the camera and the lights
    #define PIN_ERROR_LED 9							// output PIN for the red error LED, Controllino Pin D5
    #define PIN_LIGHTING_PNP 3					// output PIN to be connect to the lights STROBE_IN, Controllino Pin D6
    #define PIN_CAMERA_TRIGGER_IN 5			// output PIN to be connected to the camera's TRIGGER_IN(+) PIN, Controllino Pin D7
    #define PIN_CAMERA_STROBE_OUT 12				// input interrupt PIN IN1 to be connected to the camera's STROBE_OUT indicating the exposure time.
    #define PIN_FAN 7								// output PIN for fan, Arduino PIN_D4, Controllino D3

    #define PIN_DAISY_OUT0 10						// Daisy chain output pin
    #define PIN_DAISY_OUT1 11						// Daisy chain output pin
    #define PIN_DAISY_OUT2 13						// Daisy chain output pin
    #define PIN_DAISY_IN0 2							// interruptible PIN, Daisy Chain Input Pin
    #define PIN_DAISY_IN1 PIN_A0					// Daisy Chain Input Pin
    #define PIN_DAISY_IN2 PIN_A1					// Daisy Chain Input Pin
#endif


#endif /* GPPINS_H_ */