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

add_executable(submix main.c ap_mix.c inbetwee.c mixmerge.c
        mixprepro.c  mixshuf0.c mixshuf1.c mixshuf2.c newmix0.c
        newmix1.c setupmix.c syncatt.c)

target_link_libraries(submix cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(submix)

