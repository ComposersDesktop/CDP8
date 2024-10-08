if(APPLE)
# -mmacosx-version-min=10.9
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix -fomit-frame-pointer -funroll-loops")
  include_directories ( /Developer/Headers/FlatCarbon )
  find_library(COREAUDIOLIB CoreAudio)
  find_library(AUDIOTOOLBOX AudioToolbox)
  find_library(AULIB AudioUnit)
  find_library(CARBONLIB Carbon)
  link_directories (/usr/local/lib)
  find_library(PA NAMES libportaudio.a)
  set(EXTRA_LIBRARIES1 ${PA} ${COREAUDIOLIB} ${AUDIOTOOLBOX} ${AULIB} ${CARBONLIB} ${EXTRA_LIBRARIES})
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops")
    set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -DUSE_ASIO -fomit-frame-pointer  -funroll-loops -static-libgcc -static-libstdc++")
    link_directories(/usr/local/lib)
    find_library(PA NAMES libportaudio.a)
#    NB winmm must appear after any module that references it. Yes, that's weird.
    set(EXTRA_LIBRARIES1 dsound winspool ${PA} winmm ${EXTRA_LIBRARIES})
  else()
    include_directories(/usr/include/sound)
    link_directories (/usr/local/lib)
    find_library(PA NAMES libportaudio.a)
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
    set(EXTRA_LIBRARIES1 pthread ${PA} ${EXTRA_LIBRARIES} jack asound)
  endif()
endif()

link_directories(../../lib)

include_directories( ../../../include ../include ../portaudio/include ../portaudio/src/common )
if(MINGW)
add_executable(listaudevs devs.c cmakedummy.cpp)
else()
add_executable(listaudevs devs.c)
endif()
target_link_libraries(listaudevs ${EXTRA_LIBRARIES1})

my_install(listaudevs)
