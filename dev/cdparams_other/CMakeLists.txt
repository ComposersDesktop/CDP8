if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(cdparams_other cdparams_other.c)

target_link_libraries(cdparams_other cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(cdparams_other)
