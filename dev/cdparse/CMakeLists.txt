# DEVELOPMENT for newsfsys

if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -mmacosx-version-min=10.8 -Dunix -DIS_CDPARSE")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32 -DIS_CDPARSE")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix -DIS_CDPARSE")
  endif()
endif()

link_directories(../cdp2k ../newsfsys)

include_directories(../../include)

add_executable(cdparse cdparse.c)

target_link_libraries(cdparse cdp2k newsfsys ${EXTRA_LIBRARIES})

my_install(cdparse)
