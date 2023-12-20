///*******************************************
///@file storage.h
///@brief A class that will save/load user settings to permanent storage.
///      This will be a wrapper around the arduino EEPROM library.
///*******************************************

#ifndef STORAGE_H
#define STORAGE_H

#include "gpversion.h"
#include <EEPROM.h>
#include <Arduino.h>


namespace eeprom_data {
	// magic number indicates if EEPROM has been initialized correctly
	const long EEPROM_MAGIC_NUMBER = 1100+VERSION;  // magic number to indicate whether the AVR's EEPROM has been initialized already
	constexpr unsigned long EEPROM_MAX_WRITES = 50000UL;				// maximum number of writes in EEPROM before switching to next memory block


	// all configuration items contained in configuration_type are stored in EEPROM
	// the following block is the EEPROM master block that refers to the data blocks
	// purpose is to distribute the changes in EEPROM over the entire memory to leverage
	// the full lifespan. Masterblock only contains a magic number and a pointer to a "bank"
	// that stores the actual data. If the writeCounter of the bank gets too high, the next bank
	// is allocated in the master block and used from now on.
	struct eeprom_master_type {
		uint16_t magic_number;
		uint16_t mem_bank_address;
	};

	// This is the configuration memory block (the aforementioned "bank"). There is always one
	// active memory bank, its location in EPPROM changes whenever the write counter gets too high
	struct configuration_type {
		uint16_t write_counter;						// counts the write operation to change the EPPROM address when overflow
		bool auto_mode_on;							// if true, light is derived out of exposure time
		uint16_t no_of_strobes;						// no of pulses in a full cycle
		bool external_trigger_mode;					// if true, we are in external trigger mode

		unsigned long full_cycle_len_us;			// now_us [us] between two images. In normal operations, anything between 1000000/3 fps and 100000/20 fps is allowed
		unsigned long lights_pulse_len_us;			// [us] length of one light pulse + the break afterwards = 20ms
		unsigned long light_pulse_duty_len_us;		// [us] length of a duty cycle of one pulse, like 1ms, always < lights_pulse_len_us

	};
} // namespace eeprom_data

class Storage
{
	public:
	Storage();

	/// @brief Copy the config struct to the storage class,
	///			this is used during setup to initialize the storage class
	/// @param config - Struct with all config settings
	void set_conf(eeprom_data::configuration_type& config);

	void setup();
	bool updateStorage();
	void write();
	void read();

	void init_header();
	void write_header();
	void read_header();

	void set_byte_index(long idx);

	/// @brief returns a string with all config settings
	String getSettingsString();

	eeprom_data::configuration_type config_;
	eeprom_data::eeprom_master_type eeprom_master_block;

	private:
	long current_config_byte_to_write = -1;
	void writeByte(uint16_t no_of_byte);

};


#endif // STORAGE_H

