cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(02_control)

pico_sdk_init()

add_executable(02_control
	main.c
	${CMAKE_CURRENT_SOURCE_DIR}/protocol-task/protocol-task.c
    	${CMAKE_CURRENT_SOURCE_DIR}/stdio-task/stdio-task.c   
    	${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c
)

target_link_libraries(02_control
    pico_stdlib
)

pico_set_linker_script(02_control ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld)

pico_enable_stdio_uart(02_control 0)
pico_enable_stdio_usb(02_control 1)

pico_add_extra_outputs(02_control)