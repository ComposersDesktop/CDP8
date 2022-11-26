if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -mmacosx-version-min=10.5 -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../sfsys)

include_directories(../../include ../sfsys)

add_executable(dirsf dirsf.c wildcard.c)

target_link_libraries(dirsf cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(dirsf)

