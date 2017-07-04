cmake_minimum_required (VERSION 2.6)
project (OWON-SDS7102)

include(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})

add_library (owon-sds7102 SHARED usb.c parse.c)
set_property(TARGET owon-sds7102 PROPERTY VERSION 0.1.0)

add_executable (owon-dump owon-dump.c)
target_link_libraries(owon-dump owon-sds7102 ${LIBUSB_LIBRARIES})

add_executable (owon-parse owon-parse.c)
target_link_libraries(owon-parse owon-sds7102 ${LIBUSB_LIBRARIES})

install(TARGETS owon-sds7102 DESTINATION lib)
install(TARGETS owon-dump DESTINATION bin)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -DDEBUG_KNOWN -DDEBUG_UNKNOWN")

#Messages
message("")
message("-- Summary:")
message("* Compilation type: CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("* Install directory: CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
message("")
