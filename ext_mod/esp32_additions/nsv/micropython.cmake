# Create an INTERFACE library for our C module.


add_library(usermod_nvs INTERFACE)

set(NVS_INCLUDES)
set(NVS_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/nvs.c)

# gets nvs_flash include paths
idf_component_get_property(NVS_FLASH_INCLUDES nvs_flash INCLUDE_DIRS)
idf_component_get_property(NVS_FLASH_DIR nvs_flash COMPONENT_DIR)

# sets the include paths into INCLUDES variable
if(NVS_FLASH_INCLUDES)
    list(TRANSFORM NVS_FLASH_INCLUDES PREPEND ${NVS_FLASH_DIR}/)
    list(APPEND NVS_INCLUDES ${NVS_FLASH_INCLUDES})
endif()

# Add our source files to the lib
target_sources(usermod_nvs INTERFACE ${NVS_SOURCES})

# Add include directories.
target_include_directories(usermod_nvs INTERFACE ${NVS_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_nvs)
