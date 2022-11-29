# CDP8 with PVOCEX support
if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(blur main.c ap_blur.c blur.c)

target_link_libraries(blur cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
my_install(blur)
