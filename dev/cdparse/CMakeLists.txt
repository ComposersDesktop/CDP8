# DEVELOPMENT for newsfsys

if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix -DIS_CDPARSE")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32 -DIS_CDPARSE")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix -DIS_CDPARSE")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(cdparse cdparse.c)

target_link_libraries(cdparse cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(cdparse)
