cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pots-phone-to-bluetooth)

# Add this line to include the 'src' directory as substitute to for default 'main' directory
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/src")

# Set the global log level for the application. Higher values increase the space used in flash storage. The options are:
#    - No output (CONFIG_LOG_DEFAULT_LEVEL_NONE)
#      No log output. Used to completely disable logging. (lowest)
#
#    - Error (CONFIG_LOG_DEFAULT_LEVEL_ERROR)
#      Critical errors indicating that the software cannot recover without intervention.
#
#    - Warning (CONFIG_LOG_DEFAULT_LEVEL_WARN)
#      Events that could potentially cause issues but have been handled or mitigated.
#
#    - Info (CONFIG_LOG_DEFAULT_LEVEL_INFO)
#      General information messages that describe the normal operation of the system.
#
#    - Debug (CONFIG_LOG_DEFAULT_LEVEL_DEBUG)
#      Detailed diagnostic messages intended to aid in debugging (e.g., variable values, pointer addresses).     
#
#    - Verbose (CONFIG_LOG_DEFAULT_LEVEL_VERBOSE)
#      Highly detailed and frequent debugging messages, often including internal states, that may overwhelm the output. (highest)
set(CONFIG_LOG_DEFAULT_LEVEL CONFIG_LOG_DEFAULT_LEVEL_INFO)

