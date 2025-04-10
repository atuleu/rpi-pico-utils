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
)

add_library(rp2040-utils INTERFACE)
target_sources(rp2040-utils INTERFACE ${SRC_FILES})

target_link_libraries(
	rp2040-utils INTERFACE pico_stdlib pico_multicore hardware_flash
						   hardware_pwm
)
target_include_directories(rp2040-utils INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../)
