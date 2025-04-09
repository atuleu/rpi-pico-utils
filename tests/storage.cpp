// SPDX-License_identifier:  LGPL-3.0-or-later

#include <cstdio>
#include <pico/stdlib.h>
#include <pico/time.h>

//#include <utils/FlashStorage.hpp>
//#include <utils/Log.hpp>
//#include <utils/Scheduler.hpp>

// typedef FlashStorage<uint8_t, 1, 1>  NVu8;
// typedef FlashStorage<uint16_t, 2, 1> NVu16;

int main() {
	stdio_init_all();

	constexpr uint64_t period = 1000000;
	auto               last   = get_absolute_time() - period;
	int                i      = 0;
	while (true) {
		auto now = get_absolute_time();
		if (absolute_time_diff_us(last, now) < period) {
			continue;
		}
		last += period;
		printf("coucou %d\n", i++);
	}

	// uint8_t  a{1};
	// uint16_t b{1};

	// auto last = get_absolute_time() - 2000000;

	// gpio_init(PICO_DEFAULT_LED_PIN);
	// gpio_set_dir(PICO_DEFAULT_LED_PIN, true);

	// // Logger::Get().SetLevel(Logger::Level::DEBUG);
	// //  Logger::ScheduleLogFormattingOnOtherCore();

	// // NVu8::Load(a);
	// // NVu16::Load(b);
	// printf("coucou");
	// Scheduler::Schedule(
	//     0,
	//     500000,
	//     [&a, &b, &last](absolute_time_t now) -> std::optional<int64_t> {
	// 	    bool on = gpio_get(PICO_DEFAULT_LED_PIN);
	// 	    gpio_put(PICO_DEFAULT_LED_PIN, !on);
	// 	    if (!on) {
	// 		    // Infof("coucou a:%d b:%d", a, b);
	// 	    }

	// 	    if (absolute_time_diff_us(last, now) > 3000000) {
	// 		    ++a;
	// 		    // NVu8::Save(a);
	// 		    --b;
	// 		    // NVu16::Save(b);

	// 		    last = now + 24LLU * 3600 * 1000000;
	// 	    }

	// 	    return std::nullopt;
	//     }
	// );

	// while (true) {
	// 	Scheduler::Work();
	// }
}
