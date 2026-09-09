cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DTC_OVERLAY_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/app.overlay
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(teht2)



target_sources(app PRIVATE src/main.c src/leds.c src/buttons.c src/dispatcher.c src/dispatcher.c)
target_include_directories(app PRIVATE include)

