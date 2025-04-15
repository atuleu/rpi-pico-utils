// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <pico/types.h>

class LED {
public:
	LED(uint pin);

	void Set(uint8_t level, uint pulsePeriod_us = 0);
	void Blink(uint count, uint8_t level = 255);

private:
	void work(absolute_time_t now);

	void performBlink(absolute_time_t now);
	void performPulse(absolute_time_t now);

	uint d_slice, d_channel;

	uint8_t d_level          = 0;
	uint    d_pulsePeriod_us = 0;
	uint8_t d_blinkCount     = 0;
};
