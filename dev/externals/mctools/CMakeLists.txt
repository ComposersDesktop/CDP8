if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix -fomit-frame-pointer -funroll-loops")
  set(CMAKE_CXX_FLAGS "-O2 -Wall  -Dunix -fomit-frame-pointer -funroll-loops -std=c++11 -stdlib=libc++")
#  SET(CMAKE_EXE_LINKER_FLAGS "-static")
else()
  if(MINGW OR MSVC)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops")
    if(MINGW)
        set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops -static-libgcc -static-libstdc++")
    else()
        set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops")
    endif
  else()
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
  endif()
endif()

#sfsys is used only for sfprops

link_directories(../lib ../../newsfsys ../../pvxio2)
include_directories(../../newinclude ../include)

add_executable(abfpan abfpan.cpp)
target_link_libraries(abfpan portsf ${EXTRA_LIBRARIES})

my_install(abfpan)

add_executable(abfpan2 abfpan2.cpp)
target_link_libraries(abfpan2 portsf ${EXTRA_LIBRARIES})

my_install(abfpan2)

add_executable(abfdcode abfdcode.cpp)
target_link_libraries(abfdcode portsf ${EXTRA_LIBRARIES})

my_install(abfdcode)

add_executable(fmdcode fmdcode.c fmhfuncs.c)
target_link_libraries(fmdcode portsf ${EXTRA_LIBRARIES})

my_install(fmdcode)

add_executable(channelx channel.c)
target_link_libraries(channelx portsf m ${EXTRA_LIBRARIES})

my_install(channelx)

add_executable(chorder chorder.c)
target_link_libraries(chorder portsf ${EXTRA_LIBRARIES})

my_install(chorder)

add_executable(chxformat chxformat.c)
target_link_libraries(chxformat portsf ${EXTRA_LIBRARIES})

my_install(chxformat)

add_executable(copysfx copysf.c)
target_link_libraries(copysfx portsf ${EXTRA_LIBRARIES})

my_install(copysfx)

add_executable(interlx interlx.c)
target_link_libraries(interlx portsf ${EXTRA_LIBRARIES})

my_install(interlx)

add_executable(nmix nmix.c)
target_link_libraries(nmix portsf ${EXTRA_LIBRARIES})

my_install(nmix)

add_executable(njoin njoin.c)
target_link_libraries(njoin portsf ${EXTRA_LIBRARIES})

my_install(njoin)

add_executable(rmsinfo rmsinfo.cpp)
target_link_libraries(rmsinfo portsf ${EXTRA_LIBRARIES})

my_install(rmsinfo)

add_executable(sfprops sfprops.c)
target_link_libraries(sfprops sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(sfprops)

