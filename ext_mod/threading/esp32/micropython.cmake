# Create an INTERFACE library for our C module.

add_library(usermod_threading INTERFACE)

set(THREADING_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/../common/inc
    ${CMAKE_CURRENT_LIST_DIR}
)

set(THREADING_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/multiprocessing.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/thread_event.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/thread_lock.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/thread_rlock.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/thread_semaphore.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/thread_thread.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/src/threading.c

    ${CMAKE_CURRENT_LIST_DIR}/thread_port.c
)

# Add our source files to the lib
target_sources(usermod_threading INTERFACE ${THREADING_SOURCES})

# Add include directories.
target_include_directories(usermod_threading INTERFACE ${THREADING_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_threading)
