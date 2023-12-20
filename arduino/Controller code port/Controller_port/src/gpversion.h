///***********************************************
///@file gpversion.h
///@brief A file that will contain the version number of the software.
///***********************************************

#ifndef GPVERSION_H
#define GPVERSION_H

// whenever EEPROM data structure  or the programme changes, increase this number
constexpr int VERSION = 48;
// History:
// V48: Fix numerical accuracy in computeCycleLengths()
// V47: Added external trigger support t/T to toggle over serial
// V46: NIR pulse sync support for integration
// V45: ttyUSB support, daisy chaining now supports camera frequency update
// V44: introduced propagation cycle
// V43: Increased max duty cycle
// V42: Improved measurement between Controllino and AVR pin
// V41: Daisy chaining
// V40: bugfix: increasing the frequency caused the controller to reset
// V37-39: more measurements, and adjusted accordingly
// V36: made serial command input faster
// V35: better logging and documentation
// V34: After measurements with a scope massive improvement of timing
// V33: Timing improvement
// V32: Quicker Adjustment to changes of exposure time
// V31: Changed Fan PIN to 7
// V30: Changed Fan PIN to 9
// V29: Changed Fan PIN to 8

#endif // GPVERSION_H