cmake_minimum_required(VERSION 3.27)

add_library(vgencoder INTERFACE)

target_sources(vgencoder INTERFACE
    src/vgencoder.c
)

target_include_directories(vgencoder INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vgencoder
)
