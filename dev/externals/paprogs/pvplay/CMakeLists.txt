if(APPLE)
#-mmacosx-version-min=10.9
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix  -DMAC -DPA_USE_COREAUDIO -fomit-frame-pointer -funroll-loops")
  set(CMAKE_CXX_FLAGS "-O2 -Wall  -Dunix -DMAC -DPA_USE_COREAUDIO -fomit-frame-pointer -funroll-loops -std=c++11 -stdlib=libc++")
  include_directories ( /Developer/Headers/FlatCarbon )
  find_library(COREAUDIOLIB CoreAudio)
  find_library(AUDIOTOOLBOX AudioToolbox)
  find_library(AULIB AudioUnit)
  find_library(CARBONLIB Carbon)
  #RWD only need this for APPLE?
  link_directories (/usr/local/lib)
  find_library(AAIOLIB NAMES libaaio.a)
  set(EXTRA_LIBRARIES1 pthread ${AAIOLIB} ${COREAUDIOLIB} ${AUDIOTOOLBOX} ${AULIB} ${CARBONLIB} ${EXTRA_LIBRARIES})
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer -funroll-loops")
    set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer -funroll-loops -static-libgcc -static-libstdc++")
    set(EXTRA_LIBRARIES1 ${EXTRA_LIBRARIES})
  else()
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
    set(EXTRA_LIBRARIES1 asound jack pthread ${EXTRA_LIBRARIES})
  endif()
endif()

link_directories(../portaudio/lib/.libs ../../lib  ../../../newsfsys ../../../pvxio2)
# need /usr/local/include here?
include_directories( /usr/local/include  ../../../newinclude ../include ../portaudio/include ../portaudio/src/common )

add_executable(pvplay pvplay.cpp pvoc2.cpp fmhfuncs.c pvthreads.cpp mxfft.c)
target_link_libraries(pvplay portaudio sfsys pvxio2 ${EXTRA_LIBRARIES1})

my_install(pvplay)
