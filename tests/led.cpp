#include <utils/LED.hpp>
#include <utils/Scheduler.hpp>

#include <pico.h>

int main() {

	LED::ScheduleUpdateTask();

	LED dflt{PICO_DEFAULT_LED_PIN};

	dflt.Set(255, 2 * 1000 * 1000);
	Scheduler::Get().WorkLoop();
}
