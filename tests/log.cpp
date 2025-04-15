#include <pico/stdio.h>
#include <pico/types.h>
#include <utils/Log.hpp>
#include <utils/Scheduler.hpp>

int main() {
	stdio_init_all();

	Logger::ScheduleLogFormattingOnCore1();

	Scheduler::Get().Schedule(1000 * 1000, []() {
		static int i = 0;
		// Infof("coucou %d", ++i);
	});

	Scheduler::Get().Schedule(1000, []() {
		printf("piou");

		// Logger::FormatsNextPendingLog();
	});
	Infof("hehe");

	Scheduler::Get().WorkLoop();
}
