if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW OR MSVC)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys)

include_directories(../newinclude)

add_executable(strange main.c ap_strange.c strange.c)

target_link_libraries(strange cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(strange)

