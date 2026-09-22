cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(trackball_fw C)

# Shim minimal des en-têtes ZMK requis par le driver PMW3610 hors-arbre, pour
# l'utiliser dans une application Zephyr pure (voir README). Ajouté au chemin
# d'include global afin d'être visible depuis la compilation du module.
zephyr_include_directories(zmk_shim)

target_sources(app PRIVATE
	src/main.c
	src/usb.c
)
