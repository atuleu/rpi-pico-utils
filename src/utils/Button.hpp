// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include "utils/RingBuffer.hpp"
#include <pico/types.h>

#include <utils/Scheduler.hpp>

class Button {
public:
	enum class Event {
		CLICK,
		DOUBLE_CLICK,
		TRIPLE_CLICK,
		LONG_PRESS,
		LONG_RELEASE
	};

	Button(uint pin, Scheduler &scheduler = Scheduler::Core1());

	std::optional<Event> Pending();

private:
	void work(absolute_time_t now);

	enum class State {
		IDLE,
		DEBOUNCE,
		PRESSED,
	};
	RingBuffer<Event, 8> d_events;
	uint                 d_pin;
	State                d_state = State::IDLE;
	absolute_time_t      d_last;
	uint                 d_clickCount;
};
