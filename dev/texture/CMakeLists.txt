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

add_executable(texture main.c ap_texture.c texperm.c texture1.c
                       texture2.c texture3.c texture4.c texture5.c texprepro.c) 

target_link_libraries(texture cdp2k sfsys ${EXTRA_LIBRARIES})
my_install(texture)

