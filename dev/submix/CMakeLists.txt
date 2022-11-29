if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2 )

include_directories(../newinclude)

add_executable(submix main.c ap_mix.c inbetwee.c mixmerge.c
        mixprepro.c  mixshuf0.c mixshuf1.c mixshuf2.c newmix0.c
        newmix1.c setupmix.c syncatt.c)

target_link_libraries(submix cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(submix)

