if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(spec main.c ap_simple.c simple.c)

target_link_libraries(spec cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(spec)

