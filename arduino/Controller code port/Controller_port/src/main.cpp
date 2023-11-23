//***************************************************************
// Camera and Lighting synchronisation
// Greyparrot.AI LTD
// Author: Jochen Alt
//***************************************************************

#include <Arduino.h>
#include "gpversion.h"
#include "gpPins.h"
#include "storage.h"
// watchdog ///@todo disabled for RA4M1 testing
// #include <avr/wdt.h>
#include <WDT.h>
#include "digitalWriteFast.h"


// connections to the lighting
#define IMAGE_FREQUENCY 5						// [Hz] initial frequency of camera
#define PULSING_FREQUENCY 200					// [Hz] pulse frequency of strobing
#define MAX_DUTY_LEN_US 1800					// [us] max length of duty pulse
#define MIN_DUTY_LEN_US 200						// [us] min length of duty pulse
#define MAX_DUTY_RATIO 11						// max duty ratio is 10%, according to datasheet of ODL300
#define CAMERA_TRIGGER_LEN_US 128				// [us] length of camera trigger impulse
#define CONTROLLINO_TIME_TO_GO_LOW 92			// [us] measured time of Controllino to pull a PIN up to GND
#define CONTROLLINO_TIME_TO_GO_HIGH 4			// [us] measured time of Controllino to pull a PIN down to 24V
#define PIN_SET_TIME_US 3						// [us] Arduino time to set a pint

#define LIGHTS_PULSE_ON_DELAY 20				// [us] delay of lights to reach full brightness (datasheet)
#define LIGHTS_PULSE_OFF_DELAY 20				// [us] delay of lights to reach full darkness (datasheet)
#define WATCH_DOG_WAIT WDTO_120MS

#define BAUD_RATE 460800						// fixed baud rate of serial interface
#define LIGHT_PULSE_LEN_US (1000000UL/PULSING_FREQUENCY) // [us] length of the pulse including the break (represents 50Hz)

// error codes returned over the serial interface
#define RETURN_OK 0								// command executed successfully
#define ERROR_IMAGE_FREQUENCY_OUT_OF_RANGE 1	// passed duration between two images is not between 3 and 20 fps
#define ERROR_UNKNOWN_COMMAND 2					// passed command is unknown
#define ERROR_IMAGE_NOT_TAKEN 3					// camera did not confirm via STROBE_OUT that image has been taken
#define ERROR_IMAGE_FREQUENCY_BAD 4				// camera image grabbing frequency has more than 10% deviation from target
#define ERROR_PULSE_FREQUENCY_BAD 5				// strobing frequency has more than 10% deviation from target
#define ERROR_PULSE_DUTY_LEN_BAD 6				// strobing duty len has more than 10% deviation from target
#define ERROR_PROPAGATION_OUT_OF_RANGE 7        // propagation mode out of range

#define COMPUTE_SCALING_FACTOR 2000									// scaling factor to use during computeCycleLengths() to fix numerical accuracy issues
#define COMPUTE_SCALING_FACTOR_DIV2 COMPUTE_SCALING_FACTOR/2		// as above but divided by 2

#define LED_PIN_7 35
#define LED_PIN_3 31

const uint8_t number_of_error_codes = ERROR_PROPAGATION_OUT_OF_RANGE+1;


bool power_on = false;							// true if lights and camera are turned on
bool input_power_on = false;					// true if we received a command via serial to turn on power. Will be carried out at the beginning of a cycle.
bool input_power_off = false;					// true, if we received a command via serial to turn off the power. Will be executed immediately

// this functions starts very close to the end of a long
// to simulate an overflow of micros() after 20 seconds
inline unsigned long delayedMicros() {
#ifdef DEBUG
	return micros() + (0xFFFFFFFF - 20*1000000);
#else
	return micros();
#endif
}

// global time, is updated in every cycle of loop() and used nearly everywhere
volatile unsigned long now_us = delayedMicros();


unsigned long start_cycle_time_us = 0;			// [us] start time of the current cycle [us]
volatile unsigned long input_full_cycle_len_us = 0;		// [us] input len of full image grabbing cycle with delayed write to config
unsigned long input_light_pulse_duty_len_us = 0;// [us] length of strobing pulse
unsigned long next_pulse_start_time  = 0;		// [us] precomputed next pulse start time
unsigned long next_pulse_end_time  = 0;			// [us] precomputed next pulse end time
unsigned long next_camera_off_time = 0;			// [us] precomputed time to turn off the camera trigger pulse

unsigned long interrupt_time_us = 0;			// [us] the time the interrupt function was called [us]
unsigned long last_interrupt_time_us = 0;		// [us] the time the previous interrupt function was called [us]

constexpr unsigned long TWO_SECONDS_IN_us = 2000000UL;	// [us] 2 seconds
unsigned long external_trigger_period_us = TWO_SECONDS_IN_us; 	//[us] the period of time the lights will be on after an external trigger
unsigned long cycle_time_estimate = 0;		// estimated length of the cycle for external trigger mode.

#ifdef DEBUG
bool debugging_mode = false;					// if true, each pulse is sent to Serial with a nice pattern
#endif
bool error_led_mode = false;					// if true, the error LED is supposed to shine
bool fan_mode		= false;					// if true, the fan is supposed to shine
const int PROPAGATE_POWER_ON = 1;
const int PROPAGATE_POWER_OFF = 2;
unsigned long propagation_mode = 0;				// a number of controller has been changed by master
unsigned long nth_strobe = 0;					// counting the current number of pulses within a cycle, goes up to config.no_of_strobes

// Setup storage object
Storage storage;
eeprom_data::configuration_type& config = storage.config_;

// possible states of the main loop
bool pulse_state = false;

// the following variables are used while checking of an image really has been taken
// if all become true on one cycle, camera_works is set and the flags are reset for the next cycle
// (this is done in separate variables, since in some cases, the events do not always come in in the same sequence)
bool image_capture_turned_on = false;					// becomes true when the camera gets the command to trigger by pin TRIGGER_IN
bool image_start_latch = false;					// becomes true, when camera tells that image is being taken, set by interrupt on STROBE_OUT
bool image_done_latch = false;					// becomes true, if camera tells that exposure is finished.
bool camera_works	= false;					// is true of the STROBE signal is given

// variables to measure timing
unsigned long camera_exposure_us = 0;			// measurement of camera exposure time
unsigned long camera_exposure_avr_us = 0;		// average measurement of camera exposure time
unsigned long camera_exposure_last_avr_us = 0;	// last average that was taken for frequency calibration
unsigned long camera_exposure_avr_deriv_us = 0; // derivative of change of the exposure time. Used to detect
												// if the exposure time became stable and can be used for calibration

String command = "";							// command input, used to add up characters coming from serial interface
bool command_pending = false;
// commands have a timeout when characters are coming in too slow or the final <CR> is missing.
unsigned long cmd_last_char_us = 0;				// Time when the last character has been keyed in

// Array that stores the last timestamp when an error has been thrown, used to implement a max frequency of the same error
// Without this, an error will mostly thrown forever in a high frequency overloading Serial and the controller.
uint16_t last_error_now_us_ms[number_of_error_codes+1] = {0,0,0,0,0,0,0,0,0};
void printError(uint8_t err_no) {
	uint16_t now_ms = millis();
	if ((last_error_now_us_ms[err_no] == 0) || ((now_ms - last_error_now_us_ms[err_no]) > 1000)) {
		last_error_now_us_ms[err_no] = now_ms;
		Serial.print('E');
		Serial.println(err_no);
	}
}

// self-checking: measure some frequencies and durations to see if the
// whole thing is working properly
unsigned long measure_image_capture_duration_us = 0;
unsigned long measure_pulse_cycle_duration_us = 0;
unsigned long measure_pulse_duty_duration_us = 0;

// called whenever the camera takes an image, measures the average duration between two captures
unsigned long measure_last_image_us = 0;
void measureImageCapture() {
	if (measure_last_image_us == 0) {
		measure_last_image_us = now_us;
	}
	else {
		unsigned long value_us = now_us-measure_last_image_us;

    if(!config.external_trigger_mode)
    {
      // use a complementary filter for the measurement
      measure_image_capture_duration_us = (measure_image_capture_duration_us*7168 + value_us*1024)/8192;
    }
		measure_last_image_us = now_us;
	}
}

#ifdef DEBUG
unsigned long measure_pulse_dev_us = 0;				// [us] average deviation of duty length
unsigned long measure_pulse_max_dev_us = 0;			// [us] sliding average of max duty
unsigned long measure_pulse_duty_dev_us = 0;		// [us] average deviation of duty length
unsigned long measure_pulse_duty_max_dev_us = 0;	// [us] sliding average of max duty

// called whenever a light pulse happens, measures the average duration between two pulses
unsigned long measure_pulse_start_us = 0; // [us] start time of last pulse, used in measurePulseEnd
void measurePulseStart() {
	if (measure_pulse_start_us == 0) {
		measure_pulse_start_us = now_us;
	} else {
		unsigned long value_us = now_us-measure_pulse_start_us;
		measure_pulse_start_us = now_us;
		// use a complementary filter for the measurement
		measure_pulse_cycle_duration_us = (measure_pulse_cycle_duration_us*(2048-128) + (value_us<<7)) >> 11;

		unsigned long pulse_dev = value_us > config.lights_pulse_len_us?value_us - config.lights_pulse_len_us:config.lights_pulse_len_us-value_us;
		measure_pulse_dev_us = (measure_pulse_dev_us*(2048-64) + (pulse_dev<<6)) >> 11;
		if (measure_pulse_dev_us > measure_pulse_max_dev_us) {
			measure_pulse_max_dev_us = measure_pulse_dev_us;
	}
	else
		measure_pulse_max_dev_us = (measure_pulse_max_dev_us *( 2048-16)) >> 11;
	}
}

// called whenever a light pulse happens, measures the average duration between two pulses and the average duty cycle
void measurePulseEnd() {
	unsigned long value_us = now_us-measure_pulse_start_us;
	// use a complementary filter for the measurement
	measure_pulse_duty_duration_us = (measure_pulse_duty_duration_us*(2048 -128) + (value_us<<7)) >> 11;

	unsigned long pulse_duty_dev = value_us > config.light_pulse_duty_len_us?value_us - config.light_pulse_duty_len_us:config.light_pulse_duty_len_us- value_us;
	measure_pulse_duty_dev_us = (measure_pulse_duty_dev_us*(2048-64) + (pulse_duty_dev<<6))>> 11;
	if (pulse_duty_dev > measure_pulse_duty_max_dev_us) {
		measure_pulse_duty_max_dev_us = pulse_duty_dev;
	}
	else
		measure_pulse_duty_max_dev_us = (measure_pulse_duty_max_dev_us*(2048-16))>> 11;
}
#endif

void printMeasurements() {
  Serial.println(F("Validation"));

  if(config.external_trigger_mode)
  {
    measure_image_capture_duration_us = cycle_time_estimate;
  }
  Serial.print(F("  len between two images  : "));
  Serial.print(measure_image_capture_duration_us);
  Serial.print(F("[us] = "));
  Serial.print(1000000UL/measure_image_capture_duration_us);
  Serial.println(F("Hz"));

  Serial.print(F("  len of light pulse      : "));
  Serial.print(measure_pulse_cycle_duration_us);
  Serial.print(F("[us] = "));
  Serial.print(1000000UL/measure_pulse_cycle_duration_us);
#ifdef DEBUG
  Serial.print(F("Hz dev="));
  Serial.print(measure_pulse_dev_us);
  Serial.print(F("[us] max="));
  Serial.print(measure_pulse_max_dev_us);
  Serial.println(F("[us]"));
#endif

  Serial.print(F("  duty len of light pulse : "));
  Serial.print(measure_pulse_duty_duration_us - CONTROLLINO_TIME_TO_GO_HIGH + CONTROLLINO_TIME_TO_GO_LOW);
#ifdef DEBUG
  Serial.print(F("[us] dev="));
  Serial.print(measure_pulse_duty_dev_us);
  Serial.print(F("[us] max="));
  Serial.print(measure_pulse_duty_max_dev_us);
  Serial.println(F("[us]"));
#endif

  Serial.print(F("  camera exposure time    : "));
  if (camera_exposure_avr_us != 0) {
    Serial.print(camera_exposure_avr_us);
    Serial.print(F("[us] = 1/"));
    Serial.print(1000000UL/(camera_exposure_avr_us));
    Serial.println(F("s"));
  }

  Serial.println();
}


void computeCycleLengths() {
	// compute scaled versions of required values to improve numerical accuracy
	unsigned long scaled_full_cycle_len_us = COMPUTE_SCALING_FACTOR * config.full_cycle_len_us;
	unsigned long scaled_no_of_strobes = scaled_full_cycle_len_us/config.lights_pulse_len_us;
	unsigned long quantised_scaled_no_of_strobes = scaled_no_of_strobes / COMPUTE_SCALING_FACTOR; // the number of strobes per cycle
	if ((scaled_no_of_strobes - quantised_scaled_no_of_strobes * COMPUTE_SCALING_FACTOR)>COMPUTE_SCALING_FACTOR_DIV2) {
		// round up!
		quantised_scaled_no_of_strobes++;
	}
	config.no_of_strobes = quantised_scaled_no_of_strobes;
	config.lights_pulse_len_us = config.full_cycle_len_us/config.no_of_strobes; // now adapt the pulse cycle length to get an equal distribution of pulse
	config.light_pulse_duty_len_us = config.lights_pulse_len_us/MAX_DUTY_RATIO;

	// reset start time of measurement, so first cycle is not measured
	measure_last_image_us = 0;
#ifdef DEBUG
	measure_pulse_start_us = 0;
#endif
	measure_image_capture_duration_us = config.full_cycle_len_us;
	measure_pulse_cycle_duration_us = config.lights_pulse_len_us;
	measure_pulse_duty_duration_us = config.light_pulse_duty_len_us;

}

inline void computePulseStartTime() {
	unsigned long start_time = start_cycle_time_us + nth_strobe * config.lights_pulse_len_us;
	next_pulse_start_time  = start_time - CONTROLLINO_TIME_TO_GO_HIGH;
	next_pulse_end_time  = start_time + config.light_pulse_duty_len_us - CONTROLLINO_TIME_TO_GO_LOW;
	next_camera_off_time = start_time + CAMERA_TRIGGER_LEN_US - CONTROLLINO_TIME_TO_GO_LOW;
}


// configuration values are stored in eeprom_master_block.
// Writing to EPPROM is expensive (3ms per write) so the
// configuration struct is written bytewise in the breaks of a pulse.
// delayedWriteConfiguration start this process,
// every increment is supposed to call updateEPROMWrite
long current_config_byte_to_write = -1;							// number of byte of config which is currently written

void delayedWriteConfiguration() {
	storage.set_byte_index(0);	// start delayed write
	config.write_counter++;				// mark an additional write cycle
}

#define DO_NIR_TRIGGER

#ifdef DO_NIR_TRIGGER

#define PIN_NIR_TRIGGER_IN 13                   // output PIN to be connected to the NIR camera's trigger pin

#define NIR_TRIGGER_LEN_US 1200                  // [us] length of the NIR camera trigger pulse
#define CONTROLLINO_TIME_TO_GO_LOW_3V 50		// [us] measured time of ATMega328p to pull a PIN down to GND
#define CONTROLLINO_TIME_TO_GO_HIGH_3V 4		// [us] measured time of ATMega328p to pull a PIN up to 3.3V

bool nir_trigger_state = 0;
const unsigned long nir_trigger_factor = 10;                // the NIR trigger frequency is a factor of the camera frame rate
unsigned long nth_stripe = 0;
unsigned long nir_trigger_cycle_len_us = 0;
unsigned long next_nir_trigger_start_time = 0;
unsigned long next_nir_trigger_end_time = 0;


inline void computeNirTriggerStartTime() {
	unsigned long start_time = start_cycle_time_us + nth_stripe * (nir_trigger_cycle_len_us);
	next_nir_trigger_start_time = start_time - CONTROLLINO_TIME_TO_GO_HIGH_3V;
	next_nir_trigger_end_time = next_nir_trigger_start_time + NIR_TRIGGER_LEN_US - CONTROLLINO_TIME_TO_GO_LOW_3V;
}

inline void computeNirCycleLengths() {
	nir_trigger_cycle_len_us = config.full_cycle_len_us / nir_trigger_factor;
}
#endif // DO_NIR_TRIGGER


bool trigger_return_configuration = false;
void returnConfiguration() {
  Serial.print('V');
  Serial.print(VERSION);
  Serial.print(',');
  Serial.print(config.full_cycle_len_us);
  Serial.print(',');
  Serial.print(config.lights_pulse_len_us);
  Serial.print(',');
  Serial.print(config.light_pulse_duty_len_us);
  Serial.print(',');
  Serial.print(power_on);
  Serial.print(',');
  Serial.print(config.auto_mode_on);
  Serial.print(',');
  Serial.print(camera_works);
  Serial.print(',');
  Serial.print(error_led_mode);
  Serial.print(',');
  Serial.print(fan_mode);
  Serial.print(',');
  Serial.print(propagation_mode);
  Serial.print(',');
  Serial.print(config.external_trigger_mode);
  Serial.println();

}


/******************************/
/* Daisy Chain functionality  */
/******************************/

// Daisy chaining works with 3 pins that connect a master and its slave.
// The controller at the very beginning is master only, the next controller
// is slave to the first and master to the next. The last controller is slave only.
//
// Communication happens via a clock line and two additional data lines.
// the clock line reacts on rising and falling edge.
// Whenever clock lie is changing, the interrupt is fired, retrieves the data lines and
// acts accordingly.

// Additional functionality added in V45:
// Daisy Chain sync provides synchronization of the start of each camera & strobing cycle, but
// does not provide a mechanism to adjust the length of the cycle. This means that, for example,
// both the "leader" and "follower" units must be configured with the same camera frequency in order
// to work correctly. If the "leader" unit is then set to a different frequency then the system will
// start to work incorrectly.
// New functionality in V45 tracks the time between receipt of successive daisy chain sync pulses in
// the "follower" unit and if this deviates too far from expectation adjusts the frequency of the
// camera cycle to try and match the master. This happens automatically.

#define DAISY_INPUT_NOP 0
#define DAISY_INPUT_CYCLE_START 1		// daisy chain command to start the cycle and turn the power on
#define DAISY_INPUT_POWER_OFF 7			// command to turn off the power and stop synchronizing. Also used to check if all PINS are connected properly


// a table giving supported camera frequencies for daisy chain automatic camera frequency updates.
// We support from 1 FPS to 25 FPS inclusive, in 1 FPS increments
const uint8_t daisy_chain_number_of_cam_freq = 25;
unsigned long daisy_chain_cam_freq_cycle_times[daisy_chain_number_of_cam_freq] = {1000000,500000,333332,250000,200000,
																		166664,142856,125000,111108,100000,
																		90908,83332,76920,71428,66664,
																		62500,58820,55552,52628,50000,
																		47616,45452,43476,41664,40000};

int daisy_chain_cycle_err = 0; // a counter giving direction of detected frequency drift. Zero = no drift
uint8_t daisy_chain_camera_freq_index = 0; // index of the current camera frequency, as provided in the table daisy_chain_cam_freq_cycle_times
unsigned long daisy_chain_camera_freq,daisy_chain_camera_freq_above,daisy_chain_camera_freq_below; // current, above and below values from daisy_chain_cam_freq_cycle_times table

volatile bool daisy_chain_slave = false;	// indicates if we received a command from our master in the last cycle. Will be reset after every cycle and set with every master command.
unsigned long prev_triggered_cycle_time_us = 0; // time of the previous INO trigger event. This is used for both daisy chain and external trigger mode
bool has_cycle_start_triggered = false;  // When the interupt function is triggered we will set this to true, it will be reset in the loop() when handled.

void setDaisyChainOutput (uint8_t data) {
	digitalWriteFast(PIN_DAISY_OUT2, (data & 4)?HIGH:LOW);
	digitalWriteFast(PIN_DAISY_OUT1, (data & 2)?HIGH:LOW);

	// set interrupt pin last and have a break to be sure that after the digital isolator this comes last
	delayMicroseconds(1);
	digitalWriteFast(PIN_DAISY_OUT0, (data & 1)?HIGH:LOW);
 }

// a function to compute abs(a-b) where a and b are unsigned long
inline unsigned long absDiff(unsigned long a,unsigned long b) {
	if (a>b) {
		return a-b;
	} else {
		return b-a;
	}
}

// a function to update the current, above and below values from daisy_chain_cam_freq_cycle_times
// we copy these values from the table to avoid indexing into the array all the time. Should be faster :)
inline void daisyChainUpdateFreqValues() {
	if (daisy_chain_camera_freq_index > 0) {
		daisy_chain_camera_freq_below = daisy_chain_cam_freq_cycle_times[daisy_chain_camera_freq_index - 1];
	}
	else {
		daisy_chain_camera_freq_below = daisy_chain_cam_freq_cycle_times[0];
	}
	daisy_chain_camera_freq = daisy_chain_cam_freq_cycle_times[daisy_chain_camera_freq_index];
	if (daisy_chain_camera_freq_index < daisy_chain_number_of_cam_freq - 1) {
		daisy_chain_camera_freq_above = daisy_chain_cam_freq_cycle_times[daisy_chain_camera_freq_index + 1];
	}
	else {
		daisy_chain_camera_freq_above = daisy_chain_cam_freq_cycle_times[daisy_chain_number_of_cam_freq - 1];
	}
}

// given a cycle time (the camera freq in ns), find the nearest in the table of supported frequencies
// for daisy chaining auto sync. Once found, update the stored current, above and below values
void daisyChainFindCameraFreq(unsigned long estimated_cycle_time) {
	unsigned long best_err = daisy_chain_cam_freq_cycle_times[0];
	daisy_chain_camera_freq_index = 0;

	for (uint8_t i = 0; i < daisy_chain_number_of_cam_freq; i++) {
		unsigned long err = absDiff(daisy_chain_cam_freq_cycle_times[i], estimated_cycle_time);
		if (err < best_err) {
			daisy_chain_camera_freq_index = i;
			best_err = err;
		}
	}
	daisyChainUpdateFreqValues();
}

void calculateFreqMismatch() {
  if ((prev_triggered_cycle_time_us > 0) && (start_cycle_time_us > prev_triggered_cycle_time_us)) {
		cycle_time_estimate = start_cycle_time_us - prev_triggered_cycle_time_us;
    unsigned long cycle_err = absDiff(daisy_chain_camera_freq, cycle_time_estimate);

	// up to 1000 us of drift is considered "within expected range". Above 1000 we start to track drift
	constexpr int MAX_DRIFT_us = 1000;
	if (cycle_err > MAX_DRIFT_us) {
	  // check for drift towards lower camera frequencies. We can only do this if we are not already
	  // at index zero, our lowest supported frequency.
	  if (daisy_chain_camera_freq_index > 0) {
		unsigned long cycle_err_below = absDiff(daisy_chain_camera_freq_below, cycle_time_estimate);
		if (cycle_err_below < cycle_err) {
		  daisy_chain_cycle_err--;
		  if (daisy_chain_cycle_err == -5) {
		    // we got five cycles drifting towards a lower frequency. Update our camera frequency
			// to the previous frequency, slowing down our cycle time.
			daisy_chain_camera_freq_index--;
			daisyChainUpdateFreqValues();
			input_full_cycle_len_us = daisy_chain_camera_freq;
			daisy_chain_cycle_err = 0;
		  }
		}
	  }
	  // check for drift towards higher camera frequencies. We can only do this if we are not already
	  // at index (daisy_chain_number_of_cam_freq-1), our highest supported frequency.
	  if (daisy_chain_camera_freq_index < daisy_chain_number_of_cam_freq - 1) {
		unsigned long cycle_err_above = absDiff(daisy_chain_camera_freq_above, cycle_time_estimate);
		if (cycle_err_above < cycle_err) {
		  daisy_chain_cycle_err++;
		  if (daisy_chain_cycle_err == 5) {
			// we got five cycles drifting towards a higher frequency. Update our camera frequency
			// to the next frequency, speeding up our cycle time.
			daisy_chain_camera_freq_index++;
			daisyChainUpdateFreqValues();
			input_full_cycle_len_us = daisy_chain_camera_freq;
			daisy_chain_cycle_err = 0;
		  }
		}
      }
    }
  }
}

void computeCycleLengthsExternalTrigger()
{
  config.full_cycle_len_us = external_trigger_period_us;
  // reset start time of measurement, so first cycle is not measured
  cycle_time_estimate = start_cycle_time_us - prev_triggered_cycle_time_us;

  config.no_of_strobes = config.full_cycle_len_us/config.lights_pulse_len_us;	// compute the number of strobes per cycle and convert to int
  config.lights_pulse_len_us = config.full_cycle_len_us/config.no_of_strobes; // now adapt the pulse cycle length to get an equal distribution of pulses
  config.light_pulse_duty_len_us = config.lights_pulse_len_us/MAX_DUTY_RATIO;

  //override the number of strobes to 3
  //With external trigger it is impossible to remove the flicker so just give up
  config.no_of_strobes = 3;

  measure_pulse_cycle_duration_us = config.lights_pulse_len_us;
  measure_pulse_duty_duration_us = config.light_pulse_duty_len_us;
}

void handleCameraStrobeLatch()
{
	if (image_capture_turned_on) {
      if (image_start_latch && image_done_latch) {
        camera_works = true;
      } else {
        camera_works = false;
      }
      // next cycle sets the trigger again
      image_capture_turned_on = false;
      image_start_latch = false;
      image_done_latch = false;
    }
}

inline void handleIN0TriggerEvent()
{
	computeCycleLengthsExternalTrigger();

	//normally we do this on the last strobe but this rarely occurs
	// in external trigger mode. So we do it on every interrupt flag.
	handleCameraStrobeLatch();

}

//This handles IN0 interrupt for external trigger or daisy chain mode
void pollIN0InterruptEvent()
{
	if(has_cycle_start_triggered){
		 if(config.external_trigger_mode) {
				handleIN0TriggerEvent();
			}
      //Daisy chain mode
		  else {
        // this section attempts to check if the leader unit is running at a different cycle time (camera frequency)
        // and if so attempts to adjust this unit to match the leader
        //Only run if we had an interrupt triggered
        calculateFreqMismatch();
    	}
    has_cycle_start_triggered = false;
    //Keep this saved for the next cycle
    prev_triggered_cycle_time_us = start_cycle_time_us;
  }
}

// callback from interrupt pin PIN_DAISY_IN0, acts as clock
void daisyChainIn() {
	bool in0 = true; //On rising edge in0 is always true
	bool in1 = digitalReadFast(PIN_DAISY_IN1);
	bool in2 = digitalReadFast(PIN_DAISY_IN2);
	uint8_t daisyChainInputData = (((uint8_t)in2) << 2) + (((uint8_t)in1) << 1) + (((uint8_t)in0));

	switch (daisyChainInputData) {
	case DAISY_INPUT_CYCLE_START:
		start_cycle_time_us = now_us;

		nth_strobe = 0;
		pulse_state = false;
		daisy_chain_slave = true;

		next_pulse_start_time = start_cycle_time_us - CONTROLLINO_TIME_TO_GO_HIGH;
		next_pulse_end_time = next_pulse_start_time + config.light_pulse_duty_len_us - CONTROLLINO_TIME_TO_GO_LOW;
		next_camera_off_time = next_pulse_start_time + CAMERA_TRIGGER_LEN_US - CONTROLLINO_TIME_TO_GO_LOW;

		if (!power_on) {
			power_on = true;
			// tell GODS to turn on power with next status call
			propagation_mode = PROPAGATE_POWER_ON;
		}

		/// Calculate the next camera Period
		has_cycle_start_triggered = true;

		break;
	case DAISY_INPUT_POWER_OFF:
		if (power_on) {
			input_power_off = true;
			// tell GODS to turn on power with next status call
			propagation_mode = PROPAGATE_POWER_OFF;
		}

		break;
		break;
	default:
#ifdef DEBUG
		// Debugging, we dont expect this situation, indicate by blinking on the LED line
		for (int i = 0; i < daisyChainInputData; i++) {
			digitalWriteFast(PIN_LIGHTING_PNP, HIGH);
			delayMicroseconds(2);
			digitalWriteFast(PIN_LIGHTING_PNP, LOW);
			delayMicroseconds(2);
		}
#endif
		break;
	}
	last_interrupt_time_us = interrupt_time_us;
}


void printConfiguration() {
	Serial.println(F("Configuration"));

	Serial.print(F("	len between two images  : "));
	Serial.print(config.full_cycle_len_us);
	Serial.print(F("[us]="));
	Serial.print(1000000UL/config.full_cycle_len_us);
	Serial.println(F("Hz"));

	Serial.print(F("	len of light pulse      : "));
	Serial.print(config.lights_pulse_len_us);
	Serial.print(F("[us]="));
	Serial.print(1000000UL/config.lights_pulse_len_us);
	Serial.println(F("Hz"));

	Serial.print(F("	duty len of light pulse : "));
	Serial.print(config.light_pulse_duty_len_us);
	Serial.println(F("[us]"));
	Serial.print(F("	number of strobes/cycle : "));
	Serial.println(config.no_of_strobes);

	Serial.print(F("	camera works            : "));
	Serial.println(camera_works);

	Serial.print(F("	power_on                : "));
	Serial.println(power_on);

	Serial.print(F("	fan_on                  : "));
	Serial.println(fan_mode);

	Serial.print(F("	propagation_mode        : "));
	Serial.println(propagation_mode);

	Serial.print(F("	error led on            : "));
	Serial.println(error_led_mode);

	Serial.print(F("	auto strobe on          : "));
	Serial.println(config.auto_mode_on);

	Serial.print(F("	EEPROM(mem_bank_address="));
	Serial.print(storage.eeprom_master_block.mem_bank_address);
	Serial.print(F(", write_counter="));
	Serial.print(config.write_counter);
	Serial.println(F(")"));

	Serial.print(F("	daisy chain freq index  : "));
	Serial.println(daisy_chain_camera_freq_index);

	Serial.print(F("	daisy chain freq value  : "));
	Serial.println(daisy_chain_camera_freq);

	Serial.print(F("	External trigger mode: "));
	Serial.println(config.external_trigger_mode);

	Serial.print(F("	External trigger period: "));
	Serial.println(external_trigger_period_us);

	Serial.println();
}

void printHelp() {
	unsigned long seconds = millis()/1000;
	unsigned hours = seconds / 3600;
	unsigned minutes = (seconds - hours*3600)/60;
	seconds = seconds - hours*3600 - minutes*60;
	Serial.print(F("Camera/Lighting Synchronization V"));
	Serial.println(VERSION);
	Serial.print("uptime ");
	Serial.print(hours);
	Serial.print("h ");
	Serial.print(minutes);
	Serial.print("m ");
	Serial.print(seconds);
	Serial.println("s ");

	Serial.print(F("Strobing Ctrl/Greyparrot.AI/Jochen Alt V"));
	Serial.print(VERSION);

	#ifdef DEBUG
	Serial.println("D");
	#else
	Serial.println("R");
	#endif

	Serial.println();

	printConfiguration();
	printMeasurements();

	Serial.println(F("Help"));
	Serial.println(F("	h         help"));
	Serial.println(F("	r         restart controller"));
	Serial.println(F("	e         save to EEPROM"));
	Serial.println(F("	s         return config string"));
	Serial.println(F("	S         set configuration"));
	Serial.println(F("	0         reset to factory settings"));
	Serial.println(F("	n/N       error LED on/off"));
	Serial.println(F("	v/V       fan on/off"));

	Serial.println(F("	d/D       debugging mode on/off"));
	Serial.println(F("	a/A       auto calibration mode"));

	Serial.println(F("	p/P       power on/off"));
	Serial.println(F("	f<Hz><CR> set frequency"));
	Serial.println(F("	l<us><CR> length of strobing pulse"));
	Serial.println(F("	b<no><CR> propagation mode"));
	Serial.println(F("	t/T 	  Enable/Disable external trigger mode"));
}

// called by the interrupt triggered by the camera's STROBE_OUT
// sets a flag to indicate that the camera worked
void cameraStrobeOut() {
	bool strobe_out = digitalReadFast(PIN_CAMERA_STROBE_OUT);

	if (strobe_out == false) {
		// exposure starts
		camera_exposure_us = now_us;
		image_start_latch = true;
#ifdef DEBUG
		if (debugging_mode)
			Serial.print('O');
#endif
	} else {
		// exposure ends
#ifdef DEBUG
		if (debugging_mode)
			Serial.print('X');
#endif
		image_done_latch = true;
		// use complementary filter to compute moving average
		// use nasty bit operations for speed, since we are in an interrupt
		unsigned long duration_us = now_us - camera_exposure_us;

		if (camera_exposure_avr_us == 0) {
			camera_exposure_avr_us = duration_us;
		}
		else {
			// we are in an interrupt and need to be as fast as possible, so use nasty shift operators
			camera_exposure_avr_deriv_us = (camera_exposure_avr_deriv_us + (camera_exposure_avr_us-duration_us)) >> 1;
			camera_exposure_avr_us = (camera_exposure_avr_us + duration_us) >> 1;
		}
	}
}

void set_default_config(eeprom_data::configuration_type& default_config)
{
	// Setup default values for configuration
	default_config.write_counter = 0;
	default_config.auto_mode_on = true;								// if true, light is derived out of exposure time
	default_config.full_cycle_len_us = 1000000UL/IMAGE_FREQUENCY;	// now_us [us] between two images. In normal operations, anything between 1000000/3 fps and 100000/20 fps is allowed
	default_config.lights_pulse_len_us = LIGHT_PULSE_LEN_US;		// [us] length of one light pulse + the break afterwards = 10ms = 100 Hz
	default_config.no_of_strobes = default_config.full_cycle_len_us/default_config.lights_pulse_len_us;	// no of pulses in a full cycle
	default_config.light_pulse_duty_len_us = default_config.lights_pulse_len_us/MAX_DUTY_RATIO;	// [us] length of a duty cycle of one pulse, like 1ms, always < lights_pulse_len_us
	default_config.external_trigger_mode = false;
}

void setLED(int led_num)
{
	// make a switch case for 0,1,3
	switch (led_num) {
		case 0:
			digitalWrite(LED_PIN_7, LOW);
			digitalWrite(LED_PIN_3, LOW);
			break;
		case 1:
			digitalWrite(LED_PIN_7, HIGH);
			digitalWrite(LED_PIN_3, LOW);
			break;
		case 2:
			digitalWrite(LED_PIN_7, LOW);
			digitalWrite(LED_PIN_3, HIGH);
			break;
	}
}

bool b1 = false;
bool b2 = false;

void setup() {

	pinMode(LED_PIN_7, OUTPUT);
	pinMode(LED_PIN_3, OUTPUT);
	pinMode(D8, OUTPUT);
	pinMode(D6, OUTPUT);

	setLED(0);
	delay(500);
	setLED(1);
	delay(500);
	setLED(2);
	delay(500);
	setLED(0);

	// pins used to communicate with lighting and camera
	pinModeFast(PIN_CAMERA_STROBE_OUT, INPUT);
	pinModeFast(PIN_LIGHTING_PNP, OUTPUT);
	pinModeFast(PIN_CAMERA_TRIGGER_IN, OUTPUT);
	pinModeFast(PIN_ERROR_LED, OUTPUT);
	pinModeFast(PIN_FAN, OUTPUT);

	// initialize the daisy chain pins
	pinModeFast(PIN_DAISY_OUT0, OUTPUT);
	pinModeFast(PIN_DAISY_OUT1, OUTPUT);
	pinModeFast(PIN_DAISY_OUT2, OUTPUT);
	digitalWriteFast(PIN_DAISY_OUT0,  LOW);
	digitalWriteFast(PIN_DAISY_OUT1,  LOW);
	digitalWriteFast(PIN_DAISY_OUT2,  LOW);
	pinModeFast(PIN_DAISY_IN0, INPUT);
	pinModeFast(PIN_DAISY_IN1, INPUT);
	pinModeFast(PIN_DAISY_IN2, INPUT);
	setDaisyChainOutput(DAISY_INPUT_NOP);


#ifdef DO_NIR_TRIGGER
	// pins used to communicate with the NIR sensor
	pinModeFast(PIN_NIR_TRIGGER_IN, OUTPUT);
	digitalWriteFast(PIN_NIR_TRIGGER_IN, LOW);
#endif
	// turn off error LED
	digitalWriteFast(PIN_ERROR_LED, LOW);

	// turn off fan
	digitalWriteFast(PIN_FAN, LOW);

	// fixed baud rate to communicate with Jetson board
	 Serial.begin(BAUD_RATE);


	// camera's trigger_in is using a raising edge
	digitalWriteFast(PIN_CAMERA_TRIGGER_IN,  LOW);
	Serial.print(F("Strobing Ctrl/Greyparrot.AI/Jochen Alt V"));
	Serial.print(VERSION);

#ifdef DEBUG
	Serial.println("D");
#else
	Serial.println("R");
#endif

	// Initialise storage
	eeprom_data::configuration_type default_config;
	set_default_config(default_config);
	// Copy values over to the config obj
	storage.set_conf(default_config);

	// If config values are already stored load them, otherwise use default
	storage.setup();

	// start at the beginning of the cycle
	if(config.external_trigger_mode)
	{
		config.full_cycle_len_us = external_trigger_period_us;
	}

	// compute initial cycle lengths from EPPROM values
	computeCycleLengths();
#ifdef DO_NIR_TRIGGER
	nth_stripe = 0;


	computeNirCycleLengths();
#endif //DO_NIR_TRIGGER
	// reset the board when wdt_reset() is not called every 120ms
	/// @note temporarily disable reset for RA4M1 chip
  /// @todo find alternate method for new chip
  // wdt_enable(WATCH_DOG_WAIT);
  	WDT.begin(120);


	trigger_return_configuration = false;
	power_on = false;
	pulse_state = false;
	nth_strobe = 0;
	image_capture_turned_on = false;
	image_start_latch = false;
	image_done_latch = false;
	start_cycle_time_us = delayedMicros() + 2*CONTROLLINO_TIME_TO_GO_HIGH;
	computePulseStartTime();
	daisyChainFindCameraFreq(config.full_cycle_len_us);

#ifdef DO_NIR_TRIGGER
	computeNirTriggerStartTime();
#endif // DO_NIR_TRIGGER


	// for quality reasons this interrupt is listening to the STROBE_OUT signal of the camera
	// to check if
	attachInterrupt(digitalPinToInterrupt( PIN_CAMERA_STROBE_OUT), cameraStrobeOut, CHANGE);
	attachInterrupt(digitalPinToInterrupt( PIN_DAISY_IN0), daisyChainIn, RISING );
	WDT.refresh();
}


inline void addCmd(char ch) {
	command += ch;
	command_pending = true;
}

inline void emptyCmd() {
	command = "";
	command_pending = false;
}
bool  execute_serial_command() {
	// if the last key is too old, reset the command after 1s (command-timeout)
	if (command_pending && (now_us - cmd_last_char_us) > 1000000) {
		emptyCmd();
	}

	// check for any input
	if (Serial.available()) {
		// store time of last character, for timeout of command
		cmd_last_char_us = now_us;

		char inputChar = Serial.read();
		switch (inputChar) {
			case 'h':
				if (command == "")
					printHelp();
				else
					addCmd(inputChar);
				break;
			case '0':
				if (command == "") {
					eeprom_data::configuration_type default_config;
					set_default_config(default_config);
					storage.set_conf(default_config);
					storage.write();
					Serial.println(RETURN_OK);
					delay(1000);  // let the watch dog reset
				}
				else
					addCmd(inputChar);
				break;

			case 'r':
				delay(1000);  // let the watch dog reset
				break;
			case 'e':
				delayedWriteConfiguration();
				Serial.println(RETURN_OK);
				break;
			case 's':
				if (command == "")
					if (power_on)
						trigger_return_configuration = true;
					else
						returnConfiguration();
				else
					addCmd(inputChar);
				break;
			case 'd':
#ifdef DEBUG
			case 'D':
				if (command == "") {
					debugging_mode = (inputChar=='d');
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;
#endif
			case 'n':
			case 'N':
				if (command == "") {
					error_led_mode= (inputChar=='n');
					digitalWriteFast(PIN_ERROR_LED, error_led_mode?HIGH:LOW);
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;

			case 'v':
			case 'V':
				if (command == "") {
					fan_mode= (inputChar=='v');
					digitalWriteFast(PIN_FAN, fan_mode?HIGH:LOW);
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;

			case 'a':
			case 'A':
				if (command == "") {
					config.auto_mode_on = (inputChar=='a');
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;

			case 'p':
			case 'P':
				if (command == "") {
					if (inputChar=='p' && !power_on)
						input_power_on = true;
					else {
						if ((inputChar =='P') && power_on)
							input_power_off = true;
					}
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;
			case 'T':
			case 't':
				if (command == "") {
					if (inputChar=='t')
					{
						config.external_trigger_mode = true;
					}
					else if (inputChar=='T')
					{
						config.external_trigger_mode = false;
					}
					Serial.println(RETURN_OK);
				}
				else
					addCmd(inputChar);
				break;
			case 13:
			case 10:
				// command to set time between two images in [us]
				// anything between 3 and 20 fps is allowed
				if (command.startsWith("f")) {
					unsigned long l = command.substring(1).toInt();
					// currently the lowest frequency is 1Hz. lower than 1Hz, and the measure would get an overflow
					// since then input_full_cycle_len_us is 1000000, and the measures are filtering by multiplying with 4096
					// if that needs to be fixed, measurements need to become more coarse grained
					if (((l >= 1) && (l <= 30))) {
						// do not set immediately but let this happen in the loop at the beginning at a cycle
						input_full_cycle_len_us  = ((1000000UL/l)>>2)<<2; // timer has a resolution of 8us, so make it dividable by
						Serial.println(RETURN_OK);
					}
					else {
						printError(ERROR_IMAGE_FREQUENCY_OUT_OF_RANGE);
					}
					emptyCmd();
				} else if (command.startsWith("l")) {
					unsigned long l = command.substring(1).toInt();
					if ((l>=50) && (l<=5000)) {
						input_light_pulse_duty_len_us  = (l>>2)<<2; // timer has a resolution of 8us, so make it dividable by 4
						Serial.println(RETURN_OK);
					}
					else {
						printError(ERROR_IMAGE_FREQUENCY_OUT_OF_RANGE);
					}
					emptyCmd();
				} else if (command.startsWith("b")) {
					unsigned long l = command.substring(1).toInt();
					if ((l >= 0) && (l <= PROPAGATE_POWER_OFF)) {
						propagation_mode = l;
					    Serial.println(RETURN_OK);
					}
					else {
						printError(ERROR_PROPAGATION_OUT_OF_RANGE);
					}
					emptyCmd();
				}

				if (command != "") {
					printError(ERROR_UNKNOWN_COMMAND);
				}
				emptyCmd();
				break;

				default:
				addCmd(inputChar);
			} // switch

			return true;

		} // if (Serial.available())
	else
		return false;
}

void loop() {

	// run main loop with a state machine controlling the camera and the lights
	now_us = delayedMicros();

	// *** take care of the lights ***
	// This is most important to happen right after measuring the time to get the most precision
	bool pulse_turned_on = false;	// true if the lights just turned on
	bool pulse_turned_off = false;	// true, if the lights just turned off
	if (pulse_state == false) {
		setLED(0);
		// the way this statement is phrased deals with an overflow of now_us
 		if (now_us - next_pulse_start_time < config.full_cycle_len_us) {
			if (power_on) {
				setLED(1);
				digitalWriteFast(PIN_LIGHTING_PNP, HIGH); //turn lights on
				if (nth_strobe == 0) {
					if (!image_capture_turned_on) {
						// this delay represents the time the lights need to be turned on
						delayMicroseconds(LIGHTS_PULSE_ON_DELAY);

						image_capture_turned_on = true;
						// indicate that the camera gets the trigger to take an image
						digitalWriteFast(PIN_CAMERA_TRIGGER_IN,  HIGH);

#ifdef DEBUG
						if (debugging_mode)
							Serial.print('o');
#endif
						measureImageCapture(); // quality assurance, measure average frequency

					}
				} else {
					if (nth_strobe == 1) {
						// reset Daisy Chain command to be prepared for setting it up next time
						setDaisyChainOutput(DAISY_INPUT_NOP);
					}
				}
			}

#ifdef DEBUG
			if (nth_strobe > 0) // following call takes 40us, dont do that in the pulse when the camera is turned on to have some buffer there
				measurePulseStart(); // quality assurance, measure average frequency

			if (debugging_mode)
				if (nth_strobe == 0)
					Serial.print('[');
				else
					Serial.print('<');
#endif
			pulse_turned_on = true;
			pulse_state = true;
		} else {
			execute_serial_command();
		}
	} else {
 		if (now_us - next_pulse_end_time < config.full_cycle_len_us) {
			if (power_on) {
				digitalWriteFast(PIN_LIGHTING_PNP, LOW);// turn lights off

				// tell your slave to start the cycle when we are at the end
				if (nth_strobe == 0) {
					setDaisyChainOutput(DAISY_INPUT_CYCLE_START);
				}

			}
#ifdef DEBUG
			if (nth_strobe > 0)
				measurePulseEnd(); // quality assurance, measure average frequency
			if (debugging_mode)
				Serial.print('>');
#endif
			if (nth_strobe < config.no_of_strobes-1) {
				nth_strobe++;
			}
			else {
				if(config.external_trigger_mode)
				{
					power_on = false;
				}
				start_cycle_time_us += config.full_cycle_len_us;
				nth_strobe = 0;
				nth_stripe = 0;
				nir_trigger_state = false;
				computeNirTriggerStartTime();
				// have we received a signal from master in the last cycle?
				if (daisy_chain_slave) {
					// reset the slave flag (to be set again when we get a new master cycle)
					daisy_chain_slave = false;

					// we give ourselves half a pulse to wait for the master's voice and tell
					// us when to start the next cycle. Afterwards, we
					// continue autonomously
					start_cycle_time_us += config.lights_pulse_len_us >> 1;
				}
			}
			pulse_turned_off = true;
			pulse_state = false;
			computePulseStartTime();
		}
	}

#ifdef DO_NIR_TRIGGER
	if (nir_trigger_state == false) {
		// the way this statement is phrased deals with an overflow of now_us
		if (now_us - next_nir_trigger_start_time < config.full_cycle_len_us) {
			if (power_on) {
					// indicate that the NIR gets the trigger to take an image
					digitalWriteFast(PIN_NIR_TRIGGER_IN,  HIGH);
			}
			nir_trigger_state = true;
		}

	} else {
		if (now_us - next_nir_trigger_end_time < config.full_cycle_len_us) {
			if (power_on) {
				digitalWriteFast(PIN_NIR_TRIGGER_IN, LOW);// turn the NIR trigger off

			}

			if (nth_stripe < nir_trigger_factor - 1) {
				nth_stripe++;
				nir_trigger_state = false;
				computeNirTriggerStartTime();
			}

		}
	}
#endif // DO_NIR_TRIGGER

    /// @todo find RA4M1 alternative
  	// wdt_reset();
	WDT.refresh();
	b1 = !b1;
	digitalWrite(D8, b1);

	if (input_power_on && (nth_strobe == 0) && (!config.external_trigger_mode)) {
		power_on = input_power_on;
		input_power_on = false;
#ifdef DEBUG
		if (debugging_mode) {
			Serial.print('+');
		}
#endif
	}

	if (input_power_off) {
		power_on = false;
		input_power_off = false;
		setDaisyChainOutput (DAISY_INPUT_POWER_OFF);
#ifdef DEBUG
		if (debugging_mode) {
			Serial.print('0');
		}
#endif
	}


	// turn off the camera trigger in the second pulse, no relevance for timing
    if (image_capture_turned_on) {
		if (power_on && (now_us - next_camera_off_time < config.full_cycle_len_us)) {
			digitalWriteFast(PIN_CAMERA_TRIGGER_IN,  LOW);
		}
	}



	// *** Auto calibration mode ***
	// take the exposure time and compute strobing frequency
	// Do this right after lights turned on to avoid flickering
	if (pulse_turned_on) {
		if ((nth_strobe == 0) && config.auto_mode_on && camera_works ) {
			// only do something if the measurement changed by no more than 4us and became stable
			// use case is change of the exposure time of the camera by its properties from the outside
			if ((abs((long)camera_exposure_avr_us - (long)camera_exposure_last_avr_us)*16 > camera_exposure_avr_us)  &&
				(camera_exposure_avr_deriv_us < 8)) {
				// the LEDs are turned on 128us longer than the camera exposure
				unsigned long new_lights_pulse_duty_len_us = camera_exposure_avr_us + LIGHTS_PULSE_ON_DELAY - LIGHTS_PULSE_OFF_DELAY;

				new_lights_pulse_duty_len_us = constrain(new_lights_pulse_duty_len_us, MIN_DUTY_LEN_US, MAX_DUTY_LEN_US);
				unsigned long new_lights_pulse_len_us = MAX_DUTY_RATIO * new_lights_pulse_duty_len_us ;
#ifdef DEBUG
				if (debugging_mode) {
					Serial.println();
					Serial.print("calibration:");
					Serial.print(camera_exposure_avr_deriv_us);
					Serial.print(",");
					Serial.print(camera_exposure_last_avr_us);
					Serial.print(",");
					Serial.print(camera_exposure_avr_us);
					Serial.print(",");
					Serial.print(new_lights_pulse_len_us);
					Serial.print(",");
					Serial.print(new_lights_pulse_duty_len_us);
					Serial.println(")");
				}
#endif
				camera_exposure_last_avr_us = camera_exposure_avr_us;
				config.lights_pulse_len_us = new_lights_pulse_len_us;
				config.light_pulse_duty_len_us = new_lights_pulse_duty_len_us;
				config.no_of_strobes = config.full_cycle_len_us/config.lights_pulse_len_us;	// compute the number of strobes per cycle and convert to int
				config.lights_pulse_len_us = config.full_cycle_len_us/config.no_of_strobes; // now adapt the pulse cycle length to get an equal distribution of pulses
				config.light_pulse_duty_len_us = config.full_cycle_len_us / MAX_DUTY_RATIO;


				computeCycleLengths();
#ifdef DO_NIR_TRIGGER
				computeNirCycleLengths();
#endif //DO_NIR_TRIGGER
				start_cycle_time_us = now_us + config.lights_pulse_len_us - input_light_pulse_duty_len_us;

				computePulseStartTime();

#ifdef DO_NIR_TRIGGER
				computeNirTriggerStartTime();
#endif //DO_NIR_TRIGGER
			}
		}
	}

	// after the pulse we have 5-9ms time for some paperwork
	if (pulse_turned_off) {
		// check after the last pulse if image has been taken at some time
		if ((nth_strobe == config.no_of_strobes-1) && power_on ) {
				handleCameraStrobeLatch();
		}

		// do one of the following tasks in their order of priority

		// Check to see if IN0 has been triggered. This will happend in daisy chain or external trigger mode
		pollIN0InterruptEvent();

		// *** Accept new input from UI in the second cycle, right after ligts turned off  ***
    //Checks for FPS change
	setLED(2);
    bool fps_not_zero = (input_full_cycle_len_us != 0);
    bool fps_has_changed = (input_full_cycle_len_us != config.full_cycle_len_us);

    //Checks for light pulse on time change (i.e. exposure time changed)
    bool light_pulse_not_zero = (input_light_pulse_duty_len_us != 0);
    bool light_pulse_has_changed = (input_light_pulse_duty_len_us != config.light_pulse_duty_len_us);

    bool not_external_trigger_mode = (!config.external_trigger_mode);

		if (( ((fps_not_zero && fps_has_changed) || (light_pulse_not_zero && light_pulse_has_changed)) && not_external_trigger_mode ))
      {
			if (input_full_cycle_len_us != 0) {
				config.full_cycle_len_us = input_full_cycle_len_us;
			}
			if (input_light_pulse_duty_len_us != 0) {
				config.light_pulse_duty_len_us = input_light_pulse_duty_len_us;
				config.light_pulse_duty_len_us = constrain(config.light_pulse_duty_len_us, MIN_DUTY_LEN_US, MAX_DUTY_LEN_US);
				config.lights_pulse_len_us = MAX_DUTY_RATIO * config.light_pulse_duty_len_us;
			}

			input_full_cycle_len_us = 0;
			input_light_pulse_duty_len_us = 0;
			nth_strobe=0;

			if(!config.external_trigger_mode)
			{
				computeCycleLengths();
			}
#ifdef DO_NIR_TRIGGER
			nth_stripe=0;
			computeNirCycleLengths();
#endif //DO_NIR_TRIGGER
			start_cycle_time_us = now_us + config.lights_pulse_len_us - input_light_pulse_duty_len_us;
			computePulseStartTime();

#ifdef DO_NIR_TRIGGER
			computeNirTriggerStartTime();
#endif //DO_NIR_TRIGGER

			daisyChainFindCameraFreq(config.full_cycle_len_us);
			delayedWriteConfiguration(); // does not actually write but triggers a successive writing process
		} else if (trigger_return_configuration) { // *** return the status string ***
			returnConfiguration();
			trigger_return_configuration = false;
		} else { // *** write stuff to EEPROM ***
			// write stuff to EEPROM in the second pulse, so more than one pulse would be nice
			// write one byte to EEPROM if there is something to write. By this, we do not affect
			// the main loop while writing since we only have approx 10m in a pulse break to do this
			storage.updateStorage();
		}
	}
}