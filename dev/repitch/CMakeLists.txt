if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix")
else()
  if(MINGW OR MSVC)
    if(MSVC)
       set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 /wd4738")
    else()
	set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
    endif()
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(repitch repitch.c main.c ap_repitch.c)

target_link_libraries(repitch cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(repitch)

