# CMakeLists.txt for the getch and kbhit functions found on Windows for posix

set(CMAKE_C_FLAGS "-O -Wall ")

include_directories(../newinclude)
set(AAIOLIB_SRCS aaio_replacement.c)

add_library(aaio ${AAIOLIB_SRCS})
