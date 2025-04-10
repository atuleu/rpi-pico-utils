#include "LED.hpp"

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <optional>
#include <pico/types.h>

#include <utils/Log.hpp>
#include <utils/Scheduler.hpp>

LED::LED(uint pin, Scheduler &scheduler)
    : d_slice{pwm_gpio_to_slice_num(pin)}
    , d_channel{pwm_gpio_to_channel(pin)}
    , d_scheduler(scheduler) {

	auto config = pwm_get_default_config();
	pwm_config_set_clkdiv(&config, float(SYS_CLK_HZ) / 255 * 1000.0f);
	pwm_config_set_wrap(&config, 255 - 1);
	gpio_set_function(pin, GPIO_FUNC_PWM);
	pwm_set_chan_level(d_slice, d_channel, 0);
	pwm_init(d_slice, &config, true);

#ifdef NDEBUG
	constexpr static uint64_t PERIOD = 25000;
#else
	constexpr static uint64_t PERIOD = 250000;
#endif

	scheduler.Schedule(10, PERIOD, [this](absolute_time_t now) {
		work(now);
		return std::nullopt;
	});
}

void LED::Set(uint8_t level, uint pulsePeriod_us) {
	d_scheduler.After(100, 0, [this, level, pulsePeriod_us]() {
		d_level          = level;
		d_pulsePeriod_us = pulsePeriod_us;
		d_blinkCount     = 0;
		if (d_pulsePeriod_us == 0) {
			pwm_set_chan_level(d_slice, d_channel, level);
		}
	});
}

void LED::Blink(uint count, uint8_t level) {
	if (count == 0) {
		Set(0);
		return;
	}

	d_scheduler.After(100, 0, [this, count, level]() {
		d_blinkCount     = count;
		d_level          = level;
		d_pulsePeriod_us = 0;
	});
}

void LED::performPulse(absolute_time_t now) {
	if (d_pulsePeriod_us == 0) {
		return;
	}
	uint8_t target = d_level;
	uint    phase  = (now % d_pulsePeriod_us) * 2;
	if (phase >= d_pulsePeriod_us) {
		phase = std::max(0U, 2 * d_pulsePeriod_us - phase);
	}

	target = phase * uint(d_level) / d_pulsePeriod_us;

	Tracef(
	    "[LED %d:%d]: phase: %d period: %d (%.2f%%) target: %d",
	    d_slice,
	    d_channel,
	    phase,
	    d_pulsePeriod_us,
	    100.0f * float(phase) / d_pulsePeriod_us,
	    target
	);
	pwm_set_chan_level(d_slice, d_channel, target);
}

constexpr static int BLINK_PULSE_LENGTH_US = 250 * 1000;

void LED::performBlink(absolute_time_t now) {
	uint phase = (now / BLINK_PULSE_LENGTH_US) % (2 * (d_blinkCount + 1));
	if (phase >= 2 * d_blinkCount) {
		pwm_set_chan_level(d_slice, d_channel, 0);
		return;
	}
	pwm_set_chan_level(d_slice, d_channel, (phase % 2) == 0 ? d_level : 0);
}

void LED::work(absolute_time_t now) {
	if (d_blinkCount > 0) {
		performBlink(now);
	} else {
		performPulse(now);
	}
}
