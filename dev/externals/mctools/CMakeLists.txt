if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -mmacosx-version-min=10.8 -Dunix -fomit-frame-pointer -funroll-loops")
  set(CMAKE_CXX_FLAGS "-O2 -Wall -mmacosx-version-min=10.8 -Dunix -fomit-frame-pointer -funroll-loops -std=c++11 -stdlib=libc++")
#  SET(CMAKE_EXE_LINKER_FLAGS "-static")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops")
  else()
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
  endif()
endif()

link_directories(../lib)

include_directories(../../../include ../include)

add_executable(abfpan abfpan.cpp)
target_link_libraries(abfpan portsf sfsys ${EXTRA_LIBRARIES})

my_install(abfpan)

add_executable(abfpan2 abfpan2.cpp)
target_link_libraries(abfpan2 portsf sfsys ${EXTRA_LIBRARIES})

my_install(abfpan2)

add_executable(abfdcode abfdcode.cpp)
target_link_libraries(abfdcode portsf sfsys ${EXTRA_LIBRARIES})

my_install(abfdcode)

add_executable(fmdcode fmdcode.c fmhfuncs.c)
target_link_libraries(fmdcode portsf sfsys ${EXTRA_LIBRARIES})

my_install(fmdcode)

add_executable(channelx channel.c)
target_link_libraries(channelx portsf sfsys m ${EXTRA_LIBRARIES})

my_install(channelx)

add_executable(chorder chorder.c)
target_link_libraries(chorder portsf sfsys ${EXTRA_LIBRARIES})

my_install(chorder)

add_executable(chxformat chxformat.c)
target_link_libraries(chxformat portsf sfsys ${EXTRA_LIBRARIES})

my_install(chxformat)

add_executable(copysfx copysf.c)
target_link_libraries(copysfx portsf sfsys ${EXTRA_LIBRARIES})

my_install(copysfx)

add_executable(interlx interlx.c)
target_link_libraries(interlx portsf sfsys ${EXTRA_LIBRARIES})

my_install(interlx)

add_executable(nmix nmix.c)
target_link_libraries(nmix portsf sfsys ${EXTRA_LIBRARIES})

my_install(nmix)

add_executable(njoin njoin.c)
target_link_libraries(njoin portsf sfsys ${EXTRA_LIBRARIES})

my_install(njoin)

add_executable(rmsinfo rmsinfo.cpp)
target_link_libraries(rmsinfo portsf sfsys ${EXTRA_LIBRARIES})

my_install(rmsinfo)

add_executable(sfprops sfprops.c)
target_link_libraries(sfprops portsf sfsys ${EXTRA_LIBRARIES})

my_install(sfprops)

