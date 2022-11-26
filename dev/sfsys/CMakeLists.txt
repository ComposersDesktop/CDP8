if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -mmacosx-version-min=10.8 -DCDP97 -Dunix -D__MAC__ -DMAC  -DFILE_AMB_SUPPORT")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -DCDP97 -DCDP99 -DFILE64_WIN -D_X86_ -DWIN32 -D_WIN32 -D__GNUWIN32__ -DFILE_AMB_SUPPORT")
  else()
    set(CMAKE_C_FLAGS "-O2 -DCDP97 -Dunix -Dlinux -D_X86_  -DFILE_AMB_SUPPORT")
  endif()
endif()

include_directories(../../include)

if(MINGW)
   set(SFSYS_SRCS sfsys.c snd.c sfdir.c osbind.c props.c shortcuts.c scandir.c alias.c ieee80.c)
else()
   set(SFSYS_SRCS sfsys.c snd.c sfdir.c osbind.c props.c ieee80.c)
endif()

add_library(sfsys ${SFSYS_SRCS})
