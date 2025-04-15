#include <cstdio>

#include <boards/pico.h>
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include <string>
#include <utils/Scheduler.hpp>

int main() {
	stdio_init_all();

	gpio_init(PICO_DEFAULT_LED_PIN);
	gpio_set_dir(PICO_DEFAULT_LED_PIN, true);

	printf(
	    "----------------------------------------------------------------------"
	    "----------\nScheduler "
	    "Test\n----------------------------------------------------------------"
	    "----------------\n"
	);

	Scheduler::InitWorkLoopOnCore1([]() {
		Scheduler::Get().Schedule(
		    2000000,
		    []() { printf("MEGAPONG\n"); },
		    {
		        .Priority = SCHEDULER_HIGH_PRIORITY,
		        .Start    = 500 * 1000,
		        .Name     = "MEGAPONG",
		    }
		);

		Scheduler::Get().Schedule(
		    500000,
		    []() {
			    static int i = 0;
			    gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
		    },
		    {.Start = 0, .Name = "LED TOGGLE"}
		);
	});

	Scheduler::Get().Schedule(
	    2000000,
	    []() { printf("MEGAPING........."); },
	    {.Priority = SCHEDULER_HIGH_PRIORITY, .Start = 0, .Name = "MEGAPING"}
	);

	Scheduler::Get().Schedule(
	    1000000,
	    []() {
		    static int i = 0;
		    printf("Ping[%d]...", ++i);
		    Scheduler::Get().After(
		        500 * 1000,
		        [j = i]() { printf("pong[%d]\n", j); },
		        {.Name = "pong"}
		    );
	    },
	    {.Start = 0, .Name = "ping"}
	);

	Scheduler::WorkLoop();
}
