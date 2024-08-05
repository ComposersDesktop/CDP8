if(APPLE)
#-mmacosx-version-min=10.9
  set(CMAKE_C_FLAGS "-std=c99  -O2 -Wall -Dunix -fomit-frame-pointer -funroll-loops")
  include_directories ( /Developer/Headers/FlatCarbon ${BREW_PORTAUDIO}/include)
  find_library(COREAUDIOLIB CoreAudio)
  find_library(AUDIOTOOLBOX AudioToolbox)
  find_library(AULIB AudioUnit)
  find_library(CARBONLIB Carbon) 
  find_library(CORESERV CoreServices)
  link_directories (/usr/local/lib ${BREW_PORTAUDIO}/lib)
  find_library(PA NAMES libportaudio.a)
  find_library(AAIOLIB NAMES libaaio.a)
  set(EXTRA_LIBRARIES1 portsf pthread portaudio  ${AAIOLIB} ${PA}  ${COREAUDIOLIB} ${AUDIOTOOLBOX} ${AULIB} ${CARBONLIB} ${CORESERV} ${EXTRA_LIBRARIES})
  
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer -funroll-loops")
    set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer  -funroll-loops -static-libgcc -static-libstdc++")
    link_directories(/usr/local/lib)
    find_library(PA NAMES libportaudio.a)
#    NB winmm must appear after any module that references it. Yes, that's weird.
    set(EXTRA_LIBRARIES1 dsound winspool portsf ${PA} winmm ${EXTRA_LIBRARIES})
  else()
    link_directories (/usr/local/lib)
    find_library(PA NAMES libportaudio.a)
    find_library(AAIOLIB NAMES libaaio.a)
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
    set(EXTRA_LIBRARIES1 portsf portaudio pthread ${AAIOLIB} ${PA} ${EXTRA_LIBRARIES} jack asound)
  endif()
endif()

link_directories(../../lib )
include_directories(${COMMON_DIR} ../../include ../include ../portaudio/include ../portaudio/src/common /usr/local/include)

if(MINGW)
add_executable(paplay paplay.c fmhfuncs.c cmakedummy.cpp)
else()
add_executable(
        paplay
        paplay.c
        fmhfuncs.c
        ${COMMON_DIR}/pa_ringbuffer.c
        $<$<PLATFORM_ID:Darwin>:${COMMON_DIR}/pa_mac_core.c>
)
endif()
target_link_libraries(paplay ${EXTRA_LIBRARIES1})

my_install(paplay)
