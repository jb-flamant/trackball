cmake_minimum_required(VERSION 3.20.0)

# Bindings devicetree propres à l'application (driver PMW3610 natif). Doit être
# défini avant find_package(Zephyr) pour que dts/bindings soit pris en compte.
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(trackball_fw C)

target_sources(app PRIVATE
	src/main.c
	src/usb.c
	drivers/pmw3610.c
)
