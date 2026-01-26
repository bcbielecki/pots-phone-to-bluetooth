cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pots-phone-to-bluetooth)

# Add this line to include the 'src' directory as substitute to for default 'main' directory
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/src")

