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

add_executable(filter main.c ap_filter.c filters0.c filters1.c fltpcon.c fltprepro.c)

target_link_libraries(filter cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(filter)

