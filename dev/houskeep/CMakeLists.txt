if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW OR MSVC)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(housekeep main.c ap_house.c channels.c clean.c dump.c dupl.c respec.c sort.c)

target_link_libraries(housekeep cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(housekeep)

