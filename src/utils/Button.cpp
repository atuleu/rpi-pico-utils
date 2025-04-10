#include "Button.hpp"
#include <hardware/gpio.h>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>

Button::Button(uint pin, Scheduler &scheduler)
    : d_pin{pin} {

	gpio_init(d_pin);
	gpio_set_dir(d_pin, false);
	d_last = get_absolute_time();
	scheduler.Schedule(500, [this](absolute_time_t now) {
	    work(now);
	    return std::nullopt;
	});
}

std::optional<Button::Event> Button::Pending() {
	if (d_events.empty()) {
		return std::nullopt;
	}
	Event e;
	d_events.pop(e);
	return e;
}

void Button::work(absolute_time_t now) {
	switch (d_state) {}
}
