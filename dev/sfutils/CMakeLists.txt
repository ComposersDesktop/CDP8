if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../newsfsys ../pvxio2)

include_directories(../newinclude ../newsfsys)

add_executable(dirsf dirsf.c wildcard.c)

target_link_libraries(dirsf sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(dirsf)

