// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include "utils/Queue.hpp"
#include "utils/RingBuffer.hpp"
#include "utils/Scheduler.hpp"
#include <pico/types.h>

class Button {
public:
	enum class Event {
		CLICK,
		DOUBLE_CLICK,
		TRIPLE_CLICK,
		PRESS_DOWN,
		RELEASE,
	};

	Button(uint pin, Scheduler &scheduler = Scheduler::Core1());

	std::optional<Event> Pending();

private:
	enum class State {
		IDLE,
		DEBOUNCE,
		PRESSED,
		LONG_PRESSED,
	};

	void work(absolute_time_t now);

	void pushEvent(Event e);

	uint                                        d_pin;
	RingBuffer<Event, 8>                        d_pendingEvents;
	State                                       d_state      = State::IDLE;
	uint                                        d_clicks     = 0;
	absolute_time_t                             d_transition = 0;
};
