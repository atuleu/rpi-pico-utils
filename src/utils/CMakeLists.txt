# SPDX-License_identifier:  LGPL-3.0-or-later

set(SRC_FILES
	Defer.hpp
	RingBuffer.hpp
	Scheduler.hpp
	Scheduler.cpp
	Queue.hpp
	Log.hpp
	Log.cpp
	FlashStorage.hpp
	FlashStorage.cpp
	Duration.cpp
	Duration.hpp
	LED.hpp
	LED.cpp
	Button.hpp
	Button.cpp
)

add_library(rpi-pico-utils INTERFACE)
target_sources(rpi-pico-utils INTERFACE ${SRC_FILES})

target_link_libraries(
	rpi-pico-utils INTERFACE pico_stdlib pico_multicore hardware_flash
							 hardware_pwm
)
target_include_directories(
	rpi-pico-utils INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../
)
