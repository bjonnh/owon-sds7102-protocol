cmake_minimum_required (VERSION 2.6)
project (OWON-SDS7102)

include(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})

# For when we split the owon-dump program in two (prog + lib)
add_library (owon-sds7102 usb.c parse.c)

add_executable (owon-dump owon-dump.c)
target_link_libraries(owon-dump ${LIBUSB_LIBRARIES} owon-sds7102)

add_executable (owon-parse owon-parse.c)
target_link_libraries(owon-parse ${LIBUSB_LIBRARIES} owon-sds7102)
