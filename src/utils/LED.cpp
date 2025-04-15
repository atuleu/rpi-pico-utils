#include "LED.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/types.h>

#include <utils/Log.hpp>
#include <utils/Scheduler.hpp>

std::vector<LED *>                  LED::s_leds;
BlockingQueue<LED::ConfigUpdate, 8> LED::s_updates;

LED::LED(uint pin)
    : d_slice{pwm_gpio_to_slice_num(pin)}
    , d_channel{pwm_gpio_to_channel(pin)} {

	auto config = pwm_get_default_config();
	pwm_config_set_clkdiv(&config, float(SYS_CLK_HZ) / 255 * 1000.0f);
	pwm_config_set_wrap(&config, 255 - 1);
	gpio_set_function(pin, GPIO_FUNC_PWM);
	pwm_set_chan_level(d_slice, d_channel, 0);
	pwm_init(d_slice, &config, true);

	s_leds.push_back(this);
}

LED::~LED() {
	std::remove(s_leds.begin(), s_leds.end(), this);
	s_leds.pop_back();
}

void LED::ScheduleUpdateTask() {

#ifdef NDEBUG
	constexpr static uint64_t PERIOD = 25000;
#else
	constexpr static uint64_t PERIOD = 250000;
#endif

	Scheduler::Get().Schedule(PERIOD, updateAllTask, {.Start = 0});
}

void LED::Set(uint8_t level, uint pulsePeriod_us) {
	s_updates.AddBlocking(ConfigUpdate{
	    .Config =
	        {.Level = level, .PulsePeriod_us = pulsePeriod_us, .BlinkCount = 0},
	    .Self = this});
}

void LED::Blink(uint count, uint8_t level) {
	if (count == 0) {
		Set(0);
		return;
	}

	s_updates.AddBlocking(ConfigUpdate{
	    .Config =
	        {
	            .Level          = level,
	            .PulsePeriod_us = 0,
	            .BlinkCount     = static_cast<uint8_t>(count),
	        },
	    .Self = this});
}

void LED::performPulse(absolute_time_t now) {
	if (d_config.PulsePeriod_us == 0) {
		return;
	}
	uint8_t target = d_config.Level;
	uint    phase  = (now % d_config.PulsePeriod_us) * 2;
	if (phase >= d_config.PulsePeriod_us) {
		phase = std::max(0U, 2 * d_config.PulsePeriod_us - phase);
	}

	target = phase * uint(d_config.Level) / d_config.PulsePeriod_us;

	Tracef(
	    "[LED %d:%d]: phase: %d period: %d (%.2f%%) target: %d",
	    d_slice,
	    d_channel,
	    phase,
	    d_config.PulsePeriod_us,
	    100.0f * float(phase) / d_config.PulsePeriod_us,
	    target
	);
	pwm_set_chan_level(d_slice, d_channel, target);
}

constexpr static int BLINK_PULSE_LENGTH_US = 250 * 1000;

void LED::performBlink(absolute_time_t now) {
	uint phase =
	    (now / BLINK_PULSE_LENGTH_US) % (2 * (d_config.BlinkCount + 1));
	if (phase >= 2 * d_config.BlinkCount) {
		pwm_set_chan_level(d_slice, d_channel, 0);
		return;
	}
	pwm_set_chan_level(
	    d_slice,
	    d_channel,
	    (phase % 2) == 0 ? d_config.Level : 0
	);
}

void LED::work(absolute_time_t now) {
	if (d_config.BlinkCount > 0) {
		performBlink(now);
	} else {
		performPulse(now);
	}
}

std::optional<int64_t> LED::updateAllTask(absolute_time_t now) {
	for (ConfigUpdate update; s_updates.TryRemove(update);) {
		update.Self->setConfig(update.Config);
	}

	for (const auto self : s_leds) {
		self->work(now);
	}

	return std::nullopt;
}

void LED::setConfig(const Config &config) {
	d_config = config;

	if (config.BlinkCount == 0 && config.PulsePeriod_us == 0) {
		pwm_set_chan_level(d_slice, d_channel, config.Level);
	}
}
