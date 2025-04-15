#include <cstdio>

#include <boards/pico.h>
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include <utils/Scheduler.hpp>

int main() {
	stdio_init_all();

	gpio_init(PICO_DEFAULT_LED_PIN);
	gpio_set_dir(PICO_DEFAULT_LED_PIN, true);

	Scheduler::InitWorkLoopOnCore1();

	printf(
	    "----------------------------------------------------------------------"
	    "----------\nScheduler "
	    "Test\n----------------------------------------------------------------"
	    "----------------\n"
	);

	Scheduler::Core1().After(
	    make_timeout_time_ms(500),
	    []() {
		    Scheduler::Get().Schedule(
		        2000000,
		        []() { printf("MEGAPONG\n"); },
		        {.Priority = SCHEDULER_HIGH_PRIORITY, .Start = 0}
		    );
	    },
	    {.Start = 0}
	);
	Scheduler::Get().Schedule(
	    2000000,
	    []() { printf("MEGAPING........."); },
	    {.Priority = SCHEDULER_HIGH_PRIORITY, .Start = 0}
	);

	Scheduler::Get().Schedule(
	    1000000,
	    []() {
		    static int i = 0;
		    printf("Ping[%d]...", ++i);
		    Scheduler::Get().After(500 * 1000, [j = i]() {
			    printf("pong[%d]\n", j);
		    });
	    },
	    {.Start = 0}
	);
	Scheduler::Core1().Schedule(
	    500000,
	    []() {
		    static int i = 0;
		    gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
	    },
	    {.Start = 0}
	);

	Scheduler::WorkLoop();
}
