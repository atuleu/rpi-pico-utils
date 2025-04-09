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

	Scheduler::Get().After(100, make_timeout_time_ms(500), []() {
		Scheduler::Core1().Schedule(10, 2000000, []() {
			printf("MEGAPONG\n");
		});
	});
	Scheduler::Get().Schedule(10, 2000000, []() {
		printf("MEGAPING.........");
	});

	Scheduler::Get().Schedule(100, 1000000, []() {
		static int i = 0;
		printf("Ping[%d]...", ++i);
		Scheduler::Get().After(0, make_timeout_time_ms(500), [j = i]() {
			printf("pong[%d]\n", j);
		});
	});
	Scheduler::Core1().Schedule(100, 500000, []() {
		static int i = 0;
		gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
	});

	Scheduler::WorkLoop();
}
