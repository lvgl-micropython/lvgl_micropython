# Create an INTERFACE library for our C module.


add_library(usermod_nvs INTERFACE)

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR})
set(SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/nvs.c)

# gets nvs_flash include paths
idf_component_get_property(nvs_flash_includes nvs_flash INCLUDE_DIRS)
idf_component_get_property(nvs_flash_dir nvs_flash COMPONENT_DIR)

# sets the include paths into INCLUDES variable
if(nvs_flash_includes)
    list(TRANSFORM nvs_flash_includes PREPEND ${nvs_flash_dir}/)
    list(APPEND INCLUDES ${nvs_flash_includes})
endif()

# Add our source files to the lib
target_sources(usermod_nvs INTERFACE ${SOURCES})

# Add include directories.
target_include_directories(usermod_nvs INTERFACE ${INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_nvs)
