set(EXAMPLES scheduler storage log led)

add_executable(test_compilation main.cpp)
target_link_libraries(test_compilation rp2040-utils)

foreach(example ${EXAMPLES})
	set(TARGET_NAME test_${example})
	set(SRC_FILE ${example}.cpp)

	add_executable(${TARGET_NAME} ${SRC_FILE})
	target_link_libraries(${TARGET_NAME} rp2040-utils)
	pico_add_extra_outputs(${TARGET_NAME})
	add_openocd_upload_target(TARGET ${TARGET_NAME})

endforeach(example ${EXAMPLES})
