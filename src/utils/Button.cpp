// SPDX-License-Identifier: LGPL-3.0+

#include "Button.hpp"
#include "utils/Scheduler.hpp"
#include <hardware/gpio.h>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>

Button::Button(uint pin, Scheduler &scheduler)
    : d_pin{pin} {

	gpio_init(d_pin);

	scheduler.Schedule(1000, [this](absolute_time_t now) {
		work(now);
		return std::nullopt;
	});
}

std::optional<Button::Event> Button::Pending() {
	if (d_pendingEvents.empty()) {
		return std::nullopt;
	}
	Event e;
	d_pendingEvents.pop(e);
	return e;
}

void Button::work(absolute_time_t now) {
	static constexpr uint64_t MULTIPLE_CLICK_MAX_PERIOD_us = 300 * 1000;
	static constexpr uint64_t LONG_PRESS_MIN_TIME_us       = 250 * 1000;
	static constexpr uint64_t DEBOUNCE_TIME_us             = 20 * 1000;

	static Event multipleClickEvent[3] = {
	    Event::CLICK,
	    Event::DOUBLE_CLICK,
	    Event::TRIPLE_CLICK,
	};

	switch (d_state) {
	case State::IDLE:
		if (d_clicks > 0 && absolute_time_diff_us(d_transition, now) >
		                        MULTIPLE_CLICK_MAX_PERIOD_us) {
			pushEvent(multipleClickEvent[d_clicks - 1]);
			d_clicks = 0;
		}

		if (gpio_get(d_pin) == true) {
			d_transition = now;
			d_state      = State::DEBOUNCE;
		}
		break;
	case State::DEBOUNCE:
		if (gpio_get(d_pin) == false) {
			d_transition = now;
			d_state      = State::IDLE;
			break;
		}

		if (absolute_time_diff_us(d_transition, now) > DEBOUNCE_TIME_us) {
			d_state      = State::PRESSED;
			d_transition = now;
		}
		break;
	case State::PRESSED:
		if (gpio_get(d_pin) == false) {
			d_clicks     = std::min(3U, d_clicks + 1);
			d_transition = now;
			d_state      = State::IDLE;
			break;
		}

		if (absolute_time_diff_us(d_transition, now) > LONG_PRESS_MIN_TIME_us) {
			if (d_clicks > 0) {
				pushEvent(multipleClickEvent[d_clicks]);
				d_clicks = 0;
			}
			pushEvent(Event::PRESS_DOWN);
			d_transition = now;
			d_state      = State::LONG_PRESSED;
		}
		break;
	case State::LONG_PRESSED:
		if (gpio_get(d_pin) == false) {
			pushEvent(Event::RELEASE);
			d_transition = now;
			d_state      = State::IDLE;
		}
		break;
	}
}
