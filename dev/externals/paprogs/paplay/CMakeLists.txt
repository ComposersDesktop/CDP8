if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix -fomit-frame-pointer -funroll-loops")
  include_directories ( /Developer/Headers/FlatCarbon )
  find_library(COREAUDIOLIB CoreAudio)
  find_library(AUDIOTOOLBOX AudioToolbox)
  find_library(AULIB AudioUnit)
  find_library(CARBONLIB Carbon)
  find_library(AAIOLIB names libaaio.a paths /usr/local/lib)
  set(EXTRA_LIBRARIES1 portsf pthread ${AAIOLIB} ${COREAUDIOLIB} ${AUDIOTOOLBOX} ${AULIB} ${CARBONLIB} ${EXTRA_LIBRARIES})
  
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer -funroll-loops")
    set(EXTRA_LIBRARIES1 winmm dsound winspool ${EXTRA_LIBRARIES})
  else()
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
    set(EXTRA_LIBRARIES1 jack asound portsf pthread ${EXTRA_LIBRARIES})
  endif()
endif()



link_directories(../../lib ../portaudio/lib/.libs  /usr/local/lib)

include_directories(../../include ../include ../portaudio/include ../portaudio/src/common /usr/local/include)

add_executable(paplay paplay.c fmhfuncs.c)
target_link_libraries(paplay portaudio.a  ${EXTRA_LIBRARIES1})

my_install(paplay)
