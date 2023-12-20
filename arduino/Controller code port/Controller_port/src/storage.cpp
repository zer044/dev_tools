///*******************************************
///@file storage.cpp
///@brief A class that will save/load user settings to permanent storage.
///      This will be a wrapper around the arduino EEPROM library.
///*******************************************

#include "storage.h"

#ifdef __AVR__
    #define EEPROM_LAST E2END
#elif _RENESAS_RA_
    #define EEPROM_LAST (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)
#endif

using namespace eeprom_data;

Storage::Storage() {
}

void Storage::set_conf(eeprom_data::configuration_type& config)
{
    current_config_byte_to_write = -1;

	config_.write_counter = 0;
    config_.auto_mode_on = config.auto_mode_on;
    config_.full_cycle_len_us = config.full_cycle_len_us;
    config_.lights_pulse_len_us = config.lights_pulse_len_us;
    config_.no_of_strobes = config.no_of_strobes;
    config_.light_pulse_duty_len_us = config.light_pulse_duty_len_us;
    config_.external_trigger_mode = config.external_trigger_mode;

}

void Storage::setup() {

    // read the master block from EEPROM
    read_header();

    /// @note If we dont use this delay the write of the magic number fails
    /// @note I dont know why and it has to be 3 seconds
    delay(3000);

    // check if the EEPROM has been initialized already
    if (eeprom_master_block.magic_number != EEPROM_MAGIC_NUMBER) {
        // if not, initialize it
        init_header();
        write_header();
        // initialize the configuration with the default values
        write();
    } else {
        read();
    }
}

// write the full config block to EEPROM
void Storage::write() {
    EEPROM.put(eeprom_master_block.mem_bank_address, config_);
}

void Storage::read() {
	EEPROM.get(eeprom_master_block.mem_bank_address, config_);
}


void Storage::writeByte(uint16_t no_of_byte) {
    if (no_of_byte < sizeof(config_)) {
        // Obtain a pointer to the config struct
        uint8_t* config_byte_ptr = reinterpret_cast<uint8_t*>(&config_);

        // Access the byte at the specified index
        uint8_t write_byte = *(config_byte_ptr + no_of_byte);

        // Write the byte to EEPROM
        EEPROM.update(no_of_byte + eeprom_master_block.mem_bank_address, write_byte);
    }
}


// called regularly in pulse breaks, writes one byte to EPPROM, avr costs ca. 3ms
bool Storage::updateStorage() {
	if (current_config_byte_to_write >= 0) {
		if (config_.write_counter >= EEPROM_MAX_WRITES) {
			// new address, starting at sizeof_eeprom and increased in steps of sizeof(config)
			eeprom_master_block.mem_bank_address += sizeof(configuration_type);
			if (eeprom_master_block.mem_bank_address + sizeof(configuration_type) >= EEPROM_LAST)
            {
                // if we reached the end of the EEPROM, start over
				eeprom_master_block.mem_bank_address = sizeof(eeprom_master_block);
            }
			current_config_byte_to_write = 0;
			config_.write_counter = 0;
		}
		writeByte(current_config_byte_to_write);
		current_config_byte_to_write++;
		if (current_config_byte_to_write >= sizeof(configuration_type)) {
			current_config_byte_to_write = -1; // finish current write operation
			// finally, once the last byte has been written and we just started a new block in EEPROM
			// mark that in the master block
			if (config_.write_counter == 0) // Initialise writting into new block
				write_header();
		}
		return true;
	}

	return false;
}

void Storage::init_header() {
    eeprom_master_block.magic_number = EEPROM_MAGIC_NUMBER;
	eeprom_master_block.mem_bank_address = sizeof(eeprom_master_type);
}

void Storage::write_header() {
        EEPROM.put(0, eeprom_master_block);
}

void Storage::read_header() {
    EEPROM.get(0, eeprom_master_block);
}

String Storage::getSettingsString() {
    String settings_string = "Settings:\n";
    settings_string += "auto_mode_on: " + String(config_.auto_mode_on) + "\n";
    settings_string += "full_cycle_len_us: " + String(config_.full_cycle_len_us) + "\n";
    settings_string += "lights_pulse_len_us: " + String(config_.lights_pulse_len_us) + "\n";
    settings_string += "no_of_strobes: " + String(config_.no_of_strobes) + "\n";
    settings_string += "light_pulse_duty_len_us: " + String(config_.light_pulse_duty_len_us) + "\n";
    settings_string += "external_trigger_mode: " + String(config_.external_trigger_mode) + "\n";
    return settings_string;
}

void Storage::set_byte_index(long idx) {
    current_config_byte_to_write = idx;
}