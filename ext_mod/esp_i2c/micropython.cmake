# Create an INTERFACE library for our C module.

add_library(usermod_esp_i2c INTERFACE)

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR})
set(SOURCES ${CMAKE_CURRENT_LIST_DIR}/esp_i2c.c)

# Add our source files to the lib
target_sources(usermod_esp_i2c INTERFACE ${SOURCES})

# Add include directories.
target_include_directories(usermod_esp_i2c INTERFACE ${INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_esp_i2c)
