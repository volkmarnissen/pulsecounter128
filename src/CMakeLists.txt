# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
FILE(GLOB_RECURSE html_sources ${CMAKE_SOURCE_DIR}/lib/common/html/*.*)

idf_component_register(SRCS ${app_sources} EMBED_FILES ${html_sources})
