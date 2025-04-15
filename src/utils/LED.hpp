// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <pico/types.h>

#include <utils/Queue.hpp>

class LED {
public:
	LED(uint pin);
	~LED();

	LED(const LED &)            = delete;
	LED(LED &&)                 = delete;
	LED &operator=(const LED &) = delete;
	LED &operator=(LED &&)      = delete;

	void Set(uint8_t level, uint pulsePeriod_us = 0);
	void Blink(uint count, uint8_t level = 255);

	static void ScheduleUpdateTask();

private:
	struct Config {
		uint8_t Level          = 0;
		uint    PulsePeriod_us = 0;
		uint8_t BlinkCount     = 0;
	};

	struct ConfigUpdate {
		LED::Config Config;
		LED        *Self;
	};

	static std::optional<int64_t> updateAllTask(absolute_time_t now);

	void work(absolute_time_t now);

	void setConfig(const Config &);

	void performBlink(absolute_time_t now);
	void performPulse(absolute_time_t now);

	static std::vector<LED *>             s_leds;
	static BlockingQueue<ConfigUpdate, 8> s_updates;

	uint   d_slice, d_channel;
	Config d_config;
};
