# Create an INTERFACE library for our C module.
add_library(usermod_spi INTERFACE)

set(SPI_INCLUDES)
set(SPI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/esp32_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/esp32_spi_bus.c
    ${CMAKE_CURRENT_LIST_DIR}/src/esp32_spi_device.c
)

# Add our source files to the lib
target_sources(usermod_spi INTERFACE ${SPI_SOURCES})

# Add include directories.
target_include_directories(usermod_spi INTERFACE ${SPI_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_spi)
