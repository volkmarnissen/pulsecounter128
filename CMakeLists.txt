cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
list(APPEND EXTRA_COMPONENT_DIRS components/esp_idf_lib_helpers)
list(APPEND EXTRA_COMPONENT_DIRS components/i2cdev)
list(APPEND EXTRA_COMPONENT_DIRS components/pcf8574)

project(pulsecounter128)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf lib/common/html/index.html TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf lib/common/html/pulse.js TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf lib/common/html/pulse.css TEXT)

