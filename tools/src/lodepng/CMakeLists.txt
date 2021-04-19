# -*- mode: CMake; tab-width: 4; indent-tabs-mode: nil; -*-

set(SOURCES
    lodepng.c
    lodepng.h
    )
add_library(lodepng ${SOURCES})
target_include_directories(lodepng PUBLIC .)
set_property(TARGET lodepng PROPERTY C_STANDARD 11)
set_property(TARGET lodepng PROPERTY C_EXTENSIONS OFF)

