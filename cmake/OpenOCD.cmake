# SPDX-License_identifier:  LGPL-3.0-or-later

find_program(OPENOCD_EXECUTABLE openocd)

function(get_ocd_version)
	execute_process(
		COMMAND ${OPENOCD_EXECUTABLE} --version ERROR_VARIABLE ocd_output
	)
	string(REGEX REPLACE "Open On-Chip Debugger ([0-9.]+).*" "\\1"
						 OPENOCD_VERSION ${ocd_output}
	)
endfunction(get_ocd_version)

if(OPENOCD_EXECUTABLE)
	get_ocd_version()
endif(OPENOCD_EXECUTABLE)

if(NOT OPENOCD_EXECUTABLE OR OPENOCD_VERSION VERSION_LESS "0.12.0")
	if(OPENOCD_EXECUTABLE)
		message(
			STATUS
				"Found OpenOCD ${OPENOCD_VERSION} but it doesn't support rp2040. Downloading Pi foundation version"
		)
	endif()

	include(ExternalProject)
	include(ProcessorCount)
	ProcessorCount(N)
	ExternalProject_Add(
		openocd
		PREFIX ${PROJECT_BINARY_DIR}/_deps/openocd
		GIT_REPOSITORY https://github.com/raspberrypi/openocd.git
		GIT_TAG sdk-2.0.0
		GIT_SHALLOW 1
		GIT_PROGRESS 1
		BUILD_IN_SOURCE 1
		UPDATE_DISCONNECTED 1
		CONFIGURE_COMMAND ./bootstrap
		COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/_deps/openocd-install
				--enable-cmsis-dap
		BUILD_COMMAND make -j ${N}
		LOG_CONFIGURE 1
		LOG_OUTPUT_ON_FAILURE 1
	)
	set(OPENOCD_EXECUTABLE
		${PROJECT_BINARY_DIR}/_deps/openocd-install/bin/openocd
	)
	set(OPENOCD_VERSION "0.12.0")
endif()

add_custom_target(
	openocd-server
	COMMAND ${OPENOCD_EXECUTABLE} -s tcl -f interface/cmsis-dap.cfg -c
			"adapter speed 5000" -f target/rp2040.cfg
)

include(CMakeParseArguments)

function(add_openocd_upload_target)
	set(options)
	set(oneValueArgs TARGET)
	set(multiValueArgs)
	cmake_parse_arguments(
		ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
	)

	if(NOT ARGS_TARGET)
		message(FATAL_ERROR "You must specify one target to upload")
	endif()

	add_custom_target(
		${ARGS_TARGET}-upload
		COMMAND
			${OPENOCD_EXECUTABLE} -s tcl #
			-f interface/cmsis-dap.cfg #
			-c "adapter speed 5000" #
			-f target/rp2040.cfg #
			-c "program $<TARGET_FILE:${ARGS_TARGET}> verify" #
			-c "reset init" #
			-c "resume" #
			-c "shutdown"
		DEPENDS ${ARGS_TARGET}
	)

endfunction()
