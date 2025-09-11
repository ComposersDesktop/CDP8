# CMakeLists.txt for libaaio.a. NB  NOT on Windows!

set(CMAKE_C_FLAGS "-O -Wall ")

include_directories(../newinclude)
# original distribution just uses aaio.c. 
# but also provides aaio1.c and aaio2.c as alternatives.
# this will itself include aaio1.c
set(AAIOLIB_SRCS aaio.c)

add_library(aaio ${AAIOLIB_SRCS})