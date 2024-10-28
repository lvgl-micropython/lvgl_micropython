


cmake_minimum_required(VERSION 3.4)

set(PROJECT_LANGUAGES C)

if(DEFINED CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_STANDARD 17)
  list(APPEND PROJECT_LANGUAGES CXX)
endif(DEFINED CMAKE_CXX_COMPILER)





project(LibVNCServer VERSION 0.9.15 LANGUAGES ${PROJECT_LANGUAGES})


enable_testing()

set(PACKAGE_NAME           "LibVNCServer")
set(FULL_PACKAGE_NAME      "LibVNCServer")
set(VERSION_SO             "1")
set(PROJECT_BUGREPORT_PATH "https://github.com/LibVNC/libvncserver/issues")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LIBVNC_DIR}/cmake/Modules/")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


option(LIBVNCSERVER_INSTALL "Generate installation target" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WITH_ZLIB "Search for the zlib compression library to support additional encodings" OFF)
option(WITH_LZO "Search for the LZO compression library to omit internal miniLZO implementation" OFF)
option(WITH_JPEG "Search for the libjpeg compression library to support additional encodings" OFF)
option(WITH_PNG "Search for the PNG compression library to support additional encodings" OFF)
option(WITH_SDL "Search for the Simple Direct Media Layer library to build an example SDL vnc client" OFF)
option(WITH_GTK "Search for the GTK library to build an example GTK vnc client" OFF)
option(WITH_LIBSSHTUNNEL "Search for libsshtunnel to build an example ssh-tunneled client" OFF)
option(WITH_THREADS "Search for a threading library to build with multithreading support" OFF)
option(PREFER_WIN32THREADS "When searching for a threading library, prefer win32 threads if they are found" OFF)
option(WITH_GNUTLS "Search for the GnuTLS secure communications library to support TLS" OFF)
option(WITH_OPENSSL "Search for the OpenSSL cryptography library to support TLS and use as crypto backend" OFF)
option(WITH_SYSTEMD "Search for libsystemd to build with systemd socket activation support" OFF)
option(WITH_GCRYPT "Search for Libgcrypt to use as crypto backend" OFF)
option(WITH_FFMPEG "Search for FFMPEG to build an example VNC to MPEG encoder" OFF)
option(WITH_TIGHTVNC_FILETRANSFER "Enable filetransfer if there is pthreads support" OFF)
option(WITH_24BPP "Allow 24 bpp" OFF)
option(WITH_IPv6 "Enable IPv6 Support" OFF)
option(WITH_WEBSOCKETS "Build with websockets support" OFF)
option(WITH_SASL "Build with SASL support" OFF)
option(WITH_XCB "Build with XCB support" OFF)
option(WITH_EXAMPLES "Build examples" OFF)
option(WITH_TESTS "Build tests" OFF)
option(WITH_QT "Build the Qt client example" OFF)


# headers needed for check_type_size()


# error out if required headers not found
if(NOT HAVE_STDINT_H)
  message(FATAL_ERROR "Could NOT find required header stdint.h")
endif()


list(APPEND CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")





# TODO:
# LIBVNCSERVER_ENOENT_WORKAROUND
# inline


set(LIBVNCSERVER_SOURCES

)



add_library(vncserver ${LIBVNCSERVER_SOURCES})

target_link_libraries(vncserver
   ${ADDITIONAL_LIBS}
)

SET_TARGET_PROPERTIES(vncserver
		PROPERTIES SOVERSION "${VERSION_SO}" VERSION "${LibVNCServer_VERSION}" C_STANDARD 90



#
# this gets the libraries needed by TARGET in "-libx -liby ..." form
#
function(get_link_libraries OUT TARGET)
    set(RESULT "")
    get_target_property(LIBRARIES ${TARGET} INTERFACE_LINK_LIBRARIES)
    foreach(LIB ${LIBRARIES})
	if("${LIB}" MATCHES ".*NOTFOUND.*")
	    continue()
	endif()
	string(REGEX REPLACE "^.*/lib" "" LIB ${LIB}) # remove leading path and "lib" name prefix
	string(REGEX REPLACE "-l" "" LIB ${LIB}) # remove leading -l
	string(REGEX REPLACE "\\.so$" "" LIB ${LIB}) # remove trailing .so
	list(APPEND RESULT "-l${LIB}")
    endforeach()
    list(REMOVE_DUPLICATES RESULT)
    string(CONCAT RESULT ${RESULT}) # back to string
    if(RESULT)
	string(REPLACE "-l" " -l" RESULT ${RESULT}) # re-add separators
    endif(RESULT)
    set(${OUT} ${RESULT} PARENT_SCOPE)
endfunction()


get_link_libraries(PRIVATE_LIBS vncserver)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/libvncserver/libvncserver.pc.cmakein ${CMAKE_CURRENT_BINARY_DIR}/libvncserver.pc @ONLY)

target_sources(




