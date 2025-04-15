#include <pico/stdio.h>
#include <pico/types.h>
#include <utils/Log.hpp>
#include <utils/Scheduler.hpp>

int main() {
	stdio_init_all();

	Scheduler::InitWorkLoopOnCore1(Logger::ScheduleLogFormatting);

	Scheduler::Get().Schedule(1000 * 1000, []() {
		static int i = 0;
		Infof("Info %d", ++i);
	});

	Scheduler::Get().Schedule(2000000, []() { Warnf("A warning"); });
	Infof("starting logging");

	Scheduler::Get().WorkLoop();
}
