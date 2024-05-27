# Create an INTERFACE library for our C module.
add_library(usermod_spi INTERFACE)

set(SPI_INCLUDES)
set(SPI_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/spi.c)

# gets driver include paths
idf_component_get_property(DRIVER_INCLUDES driver INCLUDE_DIRS)
idf_component_get_property(DRIVER_DIR driver COMPONENT_DIR)

# sets the include paths into INCLUDES variable
if(DRIVER_INCLUDES)
    list(TRANSFORM DRIVER_INCLUDES PREPEND ${DRIVER_DIR}/)
    list(APPEND SPI_INCLUDES ${DRIVER_INCLUDES})
endif()

# Add our source files to the lib
target_sources(usermod_spi INTERFACE ${SPI_SOURCES})

# Add include directories.
target_include_directories(usermod_spi INTERFACE ${SPI_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_spi)
