#ifndef CONTROLLER_ERRORS_H
#define CONTROLLER_ERRORS_H

// all settings of the controller
struct StrobingControllerSettings {
  int firmware_version;                // firmware version of controller
  float camera_freq;                   // [Hz], frequency of camera
  float lights_freq;                   // [Hz], frequency of strobing
  int lights_duty_duration;            // [s], duration of a light pulse
  bool power_on;                       // true, if power_on has been called
  bool camera_works;                   // true, if camera took a picture in the last cycle
  bool error_led_on;                   // true, if the error led indicating lost connectivity is on
  bool fan_on;                         // true, if the fan is on
  int propagation = -1;                // 1, if power on has been induced by master, 2, if power off has been induced
  bool external_trigger_mode = false;  // true, if external trigger mode is on
};

/// @note Remember to add new error codes to the description vector below
enum StrobingControllerError {
  // Error codes of the controller
  // These numeric values must stay the same for backward compatibility
  ReturnOk = 0,
  ErrorImageFrequencyOutOfRange = 1,
  ErrorUnknownCommand = 2,
  ErrorImageNotTaken = 3,
  ErrorImageFrequencyBad = 4,
  ErrorPulseFrequencyBad = 5,
  ErrorPulseDutyLenBad = 6,
  ErrorPropagationOutOfRange = 7,
  // The following error codes are not used by the controller
  // These can be changed without breaking backward compatibility
  ErrorNoClientAvailable = 8,
  UnknownControllerError = 9,
  ControllerNotConnected = 10,
  CouldNotEstablishConnection = 11,
  CommunicationFailure = 12,
  CannotBurnWhileConnected = 13,
  AVRDUDECallFailed = 14,
  HexFileNotFound = 15,
  Unknown = 16
};

constexpr int CTRL_ERROR_ENUM_MAX = 17;

// Do not build the following for arduino environment
#ifndef ARDUINO

#include <vector>
#include <string>

/// @note this must match StrobingControllerError
static const std::vector<std::pair<StrobingControllerError, std::string>> ControllerErrorCodeVec = {
  { ReturnOk, "OK" },
  { ErrorImageFrequencyOutOfRange, "Image Frequency Out of Range" },
  { ErrorPropagationOutOfRange, "Propagation Out of Range" },
  { ErrorImageNotTaken, "Image Not Taken" },
  { ErrorImageFrequencyBad, "Bad Image Frequency" },
  { ErrorPulseFrequencyBad, "Bad Pulse Frequency" },
  { ErrorPulseDutyLenBad, "Bad Pulse Duty Length" },
  { ErrorUnknownCommand, "Unknown Command" },
  { ErrorNoClientAvailable, "No Client Available" },
  { UnknownControllerError, "Unknown Controller Error" },
  { ControllerNotConnected, "Controller Not Connected" },
  { CouldNotEstablishConnection, "Could Not Establish Connection" },
  { CommunicationFailure, "Communication Failure" },
  { CannotBurnWhileConnected, "Cannot Burn While Connected" },
  { AVRDUDECallFailed, "AVRDUDE Call Failed" },
  { HexFileNotFound, "Hex File Not Found" },
  { Unknown, "Unknown Error" }
};

static std::string state_str(bool b) { return b ? "true" : "false"; }

static std::string settings_to_str(const StrobingControllerSettings& settings) {
  std::stringstream ss;
  ss << "strobing status "
     << "{ \"firmware\": " << settings.firmware_version << ", "
     << "\"frequency\": " << std::setprecision(1) << std::fixed << settings.camera_freq << ", "
     << "\"strobing\": " << std::setprecision(1) << std::fixed << settings.lights_freq << ", "
     << "\"pulse\": " << settings.lights_duty_duration << ", "
     << "\"power\": " << state_str(settings.power_on) << ", "
     << "\"camera\": " << state_str(settings.camera_works) << ", "
     << "\"error led\": " << state_str(settings.error_led_on) << ", "
     << "\"fan\": " << state_str(settings.fan_on) << ", "
     << "\"propagation\": " << settings.propagation << ", "
     << "\"external triggering\": " << state_str(settings.external_trigger_mode) << " }";

  return ss.str();
}

#endif  // Exclude for ARDUINO

#endif  // CONTROLLER_ERRORS_H
