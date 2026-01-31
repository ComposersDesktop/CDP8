if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix")
else()
  if(MINGW OR MSVC)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(grain main.c ap_grain.c grain.c grain1.c graprepro.c)

target_link_libraries(grain cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(grain)

