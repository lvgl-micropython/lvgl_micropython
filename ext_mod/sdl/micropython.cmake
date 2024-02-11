# Create an INTERFACE library for our C module.

add_library(usermod_sdl INTERFACE)

set(INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/modsdl.c
    ${CMAKE_CURRENT_LIST_DIR}/src/keyboard.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mouse.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mousewheel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/display.c
)


# Add our source files to the lib
target_sources(usermod_sdl INTERFACE ${SOURCES})

# Add include directories.
target_include_directories(usermod_sdl INTERFACE ${INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_sdl)
