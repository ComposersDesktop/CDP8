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

include_directories(../../include)

add_executable(modify main.c ap_modify.c brapcon.c delay.c gain.c pan.c 
	granula1.c radical.c strans.c )

target_link_libraries(modify cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(modify)

