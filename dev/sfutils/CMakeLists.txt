if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix -DENABLE_PVX")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DENABLE_PVX -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -DENABLE_PVX -Dunix")
  endif()
endif()

link_directories(../newsfsys ../pvxio2)

include_directories(../newinclude ../newsfsys)

add_executable(dirsf dirsf.c wildcard.c)

target_link_libraries(dirsf sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(dirsf)

