# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

# Create an INTERFACE library for our C module.
add_library(usermod_lcd_bus INTERFACE)

if(ESP_PLATFORM)

    get_filename_component(BINDING_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
    set(LIBVNC_DIR ${BINDING_DIR}/lib/libvncserver
    set(LIBVNCSERVER_DIR ${LIBVNC_DIR}/src/libvncserver)
    set(LIBVNCCLIENT_DIR ${LIBVNC_DIR}/src/libvncclient)
    set(LIBVNCCOMMON_DIR ${LIBVNC_DIR}/src/common)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LIBVNC_DIR}/cmake/Modules/")

    set(FREERTOS_POSIX_DIR ${BINDING_DIR}/lib/Lab-Project-FreeRTOS-POSIX)
    set(LWIP_DIR ${BINDING_DIR}/lib/micropythob/lib/lwip)

    include(CheckFunctionExists)
    include(CheckSymbolExists)
    include(CheckIncludeFile)
    include(CheckTypeSize)
    include(TestBigEndian)
    include(CheckCSourceCompiles)
    include(CheckCSourceRuns)
    include(GNUInstallDirs)

    set(LCD_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/esp32_include
        ${CMAKE_CURRENT_LIST_DIR}/rfb_bus
        ${LIBVNC_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${LIBVNCSERVER_DIR}
        ${LIBVNCCLIENT_DIR}
        ${LIBVNCCOMMON_DIR}
        ${FREERTOS_POSIX_DIR}/include
        ${FREERTOS_POSIX_DIR}/include/FreeRTOS_POSIX
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/include
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/include/portable/espressif/esp32_devkitc_esp_wrover_kit
        ${LWIP_DIR}/src/include/lwip
        ${LWIP_DIR}/src/compat/posix
    )

    set(LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/modlcd_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/rgb_bus.c
        #
        ${CMAKE_CURRENT_LIST_DIR}/rfb_bus/rfb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/rfb_bus/rfb_keysym.c
        #
        ${LIBVNCSERVER_DIR}/main.c
        ${LIBVNCSERVER_DIR}/rfbserver.c
        ${LIBVNCSERVER_DIR}/rfbregion.c
        ${LIBVNCSERVER_DIR}/auth.c
        ${LIBVNCSERVER_DIR}/sockets.c
        ${LIBVNCSERVER_DIR}/stats.c
        ${LIBVNCSERVER_DIR}/corre.c
        ${LIBVNCSERVER_DIR}/hextile.c
        ${LIBVNCSERVER_DIR}/rre.c
        ${LIBVNCSERVER_DIR}/translate.c
        ${LIBVNCSERVER_DIR}/cutpaste.c
        ${LIBVNCSERVER_DIR}/httpd.c
        ${LIBVNCSERVER_DIR}/cursor.c
        ${LIBVNCSERVER_DIR}/font.c
        ${LIBVNCSERVER_DIR}/draw.c
        ${LIBVNCSERVER_DIR}/selbox.c
        ${LIBVNCSERVER_DIR}/cargs.c
        ${LIBVNCSERVER_DIR}/ultra.c
        ${LIBVNCSERVER_DIR}/scale.c
        #
        ${LIBVNCCLIENT_DIR}/cursor.c
        ${LIBVNCCLIENT_DIR}/listen.c
        ${LIBVNCCLIENT_DIR}/rfbclient.c
        ${LIBVNCCLIENT_DIR}/sockets.c
        ${LIBVNCCLIENT_DIR}/vncviewer.c
        #
        ${LIBVNCCOMMON_DIR}/vncauth.c
        ${LIBVNCCOMMON_DIR}/sockets.c
        ${LIBVNCCOMMON_DIR}/sockets.c
        #
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_clock.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_mqueue.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_barrier.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_cond.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_mutex.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_sched.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_semaphore.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_timer.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_unistd.c
        ${FREERTOS_POSIX_DIR}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_utils.c
    )


    check_include_file("dirent.h"       LIBVNCSERVER_HAVE_DIRENT_H)
    check_include_file("endian.h"       LIBVNCSERVER_HAVE_ENDIAN_H)
    check_include_file("fcntl.h"        LIBVNCSERVER_HAVE_FCNTL_H)
    check_include_file("netinet/in.h"   LIBVNCSERVER_HAVE_NETINET_IN_H)
    check_include_file("sys/endian.h"   LIBVNCSERVER_HAVE_SYS_ENDIAN_H)
    check_include_file("sys/socket.h"   LIBVNCSERVER_HAVE_SYS_SOCKET_H)
    check_include_file("sys/stat.h"     LIBVNCSERVER_HAVE_SYS_STAT_H)
    check_include_file("sys/time.h"     LIBVNCSERVER_HAVE_SYS_TIME_H)
    check_include_file("sys/types.h"    LIBVNCSERVER_HAVE_SYS_TYPES_H)
    check_include_file("sys/wait.h"     LIBVNCSERVER_HAVE_SYS_WAIT_H)
    check_include_file("unistd.h"       LIBVNCSERVER_HAVE_UNISTD_H)
    check_include_file("sys/resource.h" LIBVNCSERVER_HAVE_SYS_RESOURCE_H)


    # headers needed for check_type_size()
    check_include_file("vfork.h"       LIBVNCSERVER_HAVE_VFORK_H)
    check_include_file("ws2tcpip.h"    LIBVNCSERVER_HAVE_WS2TCPIP_H)
    check_include_file("arpa/inet.h"   HAVE_ARPA_INET_H)
    check_include_file("stdint.h"      HAVE_STDINT_H)
    check_include_file("stddef.h"      HAVE_STDDEF_H)
    check_include_file("sys/types.h"   HAVE_SYS_TYPES_H)

    # error out if required headers not found
    if(NOT HAVE_STDINT_H)
      message(FATAL_ERROR "Could NOT find required header stdint.h")
    endif()

    check_function_exists(gettimeofday    LIBVNCSERVER_HAVE_GETTIMEOFDAY)
    check_function_exists(vfork           LIBVNCSERVER_HAVE_VFORK)
    check_function_exists(vprintf         LIBVNCSERVER_HAVE_VPRINTF)
    check_function_exists(mmap            LIBVNCSERVER_HAVE_MMAP)
    check_function_exists(fork            LIBVNCSERVER_HAVE_FORK)
    check_function_exists(ftime           LIBVNCSERVER_HAVE_FTIME)
    check_function_exists(gethostbyname   LIBVNCSERVER_HAVE_GETHOSTBYNAME)
    check_function_exists(gethostname     LIBVNCSERVER_HAVE_GETHOSTNAME)
    check_function_exists(inet_ntoa       LIBVNCSERVER_HAVE_INET_NTOA)
    check_function_exists(memmove         LIBVNCSERVER_HAVE_MEMMOVE)
    check_function_exists(memset          LIBVNCSERVER_HAVE_MEMSET)
    check_function_exists(mkfifo          LIBVNCSERVER_HAVE_MKFIFO)
    check_function_exists(select          LIBVNCSERVER_HAVE_SELECT)
    check_function_exists(socket          LIBVNCSERVER_HAVE_SOCKET)
    check_function_exists(strchr          LIBVNCSERVER_HAVE_STRCHR)
    check_function_exists(strcspn         LIBVNCSERVER_HAVE_STRCSPN)
    check_function_exists(strdup          LIBVNCSERVER_HAVE_STRDUP)
    check_function_exists(strerror        LIBVNCSERVER_HAVE_STRERROR)
    check_function_exists(strstr          LIBVNCSERVER_HAVE_STRSTR)

    check_symbol_exists(htobe64 "endian.h" LIBVNCSERVER_HAVE_HTOBE64)
    check_symbol_exists(htobe64 "sys/endian.h" LIBVNCSERVER_HAVE_HTOBE64)
    check_symbol_exists(OSSwapHostToBigInt64 "libkern/OSByteOrder.h" LIBVNCSERVER_HAVE_OSSWAPHOSTTOBIGINT64)

    if(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
      # socklen_t
      list(APPEND CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
    endif(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
    if(HAVE_ARPA_INET_H)
      # in_addr_t
      list(APPEND CMAKE_EXTRA_INCLUDE_FILES "arpa/inet.h")
    endif(HAVE_ARPA_INET_H)

    check_type_size(pid_t     LIBVNCSERVER_PID_T)
    check_type_size(size_t    LIBVNCSERVER_SIZE_T)
    check_type_size(socklen_t LIBVNCSERVER_SOCKLEN_T)
    check_type_size(in_addr_t LIBVNCSERVER_IN_ADDR_T)

    if(NOT HAVE_LIBVNCSERVER_IN_ADDR_T)
      set(LIBVNCSERVER_NEED_INADDR_T 1)
    endif(NOT HAVE_LIBVNCSERVER_IN_ADDR_T)

    TEST_BIG_ENDIAN(LIBVNCSERVER_WORDS_BIGENDIAN)

    configure_file(${LIBVNC_DIR}/include/rfb/rfbconfig.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/include/rfb/rfbconfig.h)

    # gets esp_lcd include paths
    idf_component_get_property(ESP_LCD_INCLUDES esp_lcd INCLUDE_DIRS)
    idf_component_get_property(ESP_LCD_DIR esp_lcd COMPONENT_DIR)

    set(LIBVNCSERVER_HAVE_LIBPTHREAD 1)
    set(LIBVNCSERVER_ALLOW24BPP 1)

    # sets the include paths into INCLUDES variable
    if(ESP_LCD_INCLUDES)
        list(TRANSFORM ESP_LCD_INCLUDES PREPEND ${ESP_LCD_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_LCD_INCLUDES})
    endif()

else()
    set(LCD_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/common_include
        ${CMAKE_CURRENT_LIST_DIR}/sdl_bus
    )

    set(LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/modlcd_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/rgb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/sdl_bus/sdl_bus.c
    )

endif(ESP_PLATFORM)


# Add our source files to the lib
target_sources(usermod_lcd_bus INTERFACE ${LCD_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_bus INTERFACE ${LCD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_bus)
