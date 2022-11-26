# CMakeLists.txt for newsfsys (Release 8) RWD 2022
if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -mmacosx-version-min=10.8  -Dunix -D__MAC__ -DMAC -DENABLE_PVX")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -D_X86_ -DWIN32 -D_WIN32 -D__GNUWIN32__ -DENABLE_PVX")
  else()
    set(CMAKE_C_FLAGS "-O2 -Dunix -Dlinux -D_X86_ -DENABLE_PVX")
  endif()
endif()

include_directories(../newinclude)

if(MINGW)
   set(SFSYS_SRCS sfsys.c snd.c sfdir.c osbind.c props.c shortcuts.c scandir.c alias.c ieee80.c)
else()
   set(SFSYS_SRCS sfsys.c snd.c sfdir.c osbind.c props.c ieee80.c pvfileio.c)
endif()

add_library(newsfsys ${SFSYS_SRCS})
