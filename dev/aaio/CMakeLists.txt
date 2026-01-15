# CMakeLists.txt for libaaio.a. NB  NOT on Windows!

set(CMAKE_C_FLAGS "-O -Wall ")

include_directories(../newinclude)
set(AAIOLIB_SRCS aaio.c)
add_library(aaio ${AAIOLIB_SRCS})
