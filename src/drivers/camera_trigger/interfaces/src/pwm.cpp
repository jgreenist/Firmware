#include <px4.h>
#include <sys/ioctl.h>
#include <lib/helpfuncs/helpfuncs.h>

#include "pwm.h"

CameraInterfacePWM::CameraInterfacePWM():
	CameraInterface(),
	_vehicle_status_sub(-1),
	_camera_is_on(false)
{
	_p_pin = param_find("TRIG_PINS");
	int pin_list;
	param_get(_p_pin, &pin_list);

	// Set all pins as invalid
	for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
		_pins[i] = -1;
	}

	// Convert number to individual channels
	unsigned i = 0;
	int single_pin;

	while ((single_pin = pin_list % 10)) {

		_pins[i] = single_pin - 1;

		if (_pins[i] < 0) {
			_pins[i] = -1;
		}

		pin_list /= 10;
		i++;
	}
}

CameraInterfacePWM::~CameraInterfacePWM()
{
}

void CameraInterfacePWM::setup()
{
	_pwm_dev = PWM_DEVICE_PATH;  // Used for direct pwm output without mixer
	_pwm_fd = open(_pwm_dev, 0);  // open pwm device

	if (_pwm_fd < 0) {
		err(1, "can't open %s", _pwm_dev);
	}

	for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
		if (_pins[i] >= 0) {
			px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_DISARMED, 1000,
					2000)); // TODO(birchera): use return value to make sure everything is fine
		}
	}
}

void CameraInterfacePWM::trigger(bool enable)
{
	// Check if armed, otherwise don't send high PWM values
	if (_vehicle_status_sub < 0) {
		_vehicle_status_sub = orb_subscribe(ORB_ID(vehicle_status));
	}

	orb_copy(ORB_ID(vehicle_status), _vehicle_status_sub, &_vehicle_status);

	if (_vehicle_status.arming_state != _vehicle_status.ARMING_STATE_ARMED
	    || !_vehicle_status.no_pwm) {
		for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
			if (_pins[i] >= 0) {
				px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_DISARMED, 1000, 2000));
			}
		}

	} else {
		if (!_camera_is_on) {
			// Turn camera on and give time to start up
			powerOn();
			return;
		}

		if (trig) {
			// Set all valid pins to shoot level
			for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
				if (_pins[i] >= 0) {
					px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_INSTANT_SHOOT, 1000, 2000));
				}
			}

		} else {
			// Set all valid pins back to neutral level
			for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
				if (_pins[i] >= 0) {
					px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_NEUTRAL, 1000, 2000));
				}
			}
		}
	}
}

int CameraInterfacePWM::powerOn()
{
	// Check if armed, otherwise don't send high PWM values
	if (_vehicle_status_sub < 0) {
		_vehicle_status_sub = orb_subscribe(ORB_ID(vehicle_status));
	}

	orb_copy(ORB_ID(vehicle_status), _vehicle_status_sub, &_vehicle_status);

	if (_vehicle_status.arming_state != _vehicle_status.ARMING_STATE_ARMED
	    || !_vehicle_status.no_pwm) {
		for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
			if (_pins[i] >= 0) {
				px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_DISARMED, 1000, 2000));
			}
		}

	} else {
		// Set all valid pins to turn on level
		for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
			if (_pins[i] >= 0) {
				px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_ON, 1000, 2000));
			}
		}

		_camera_is_on = true;
	}

	return 0;
}

int CameraInterfacePWM::powerOff()
{
	// Check if armed, otherwise don't send high PWM values
	if (_vehicle_status_sub < 0) {
		_vehicle_status_sub = orb_subscribe(ORB_ID(vehicle_status));
	}

	orb_copy(ORB_ID(vehicle_status), _vehicle_status_sub, &_vehicle_status);

	if (_vehicle_status.arming_state != _vehicle_status.ARMING_STATE_ARMED
	    || !_vehicle_status.no_pwm) {
		for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
			if (_pins[i] >= 0) {
				px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_DISARMED, 1000, 2000));
			}
		}

	} else {
		// Set all valid pins to turn off level
		for (unsigned i = 0; i < sizeof(_pins) / sizeof(_pins[0]); i++) {
			if (_pins[i] >= 0) {
				px4_ioctl(_pwm_fd, PWM_CAMERA_SET(_pins[i]), math::constrain(PWM_CAMERA_OFF, 1000, 2000));
			}
		}

		_camera_is_on = false;
	}

	return 0;
}
