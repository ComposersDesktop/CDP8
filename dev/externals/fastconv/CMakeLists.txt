if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall  -Dunix -fomit-frame-pointer -funroll-loops")
  set(CMAKE_CXX_FLAGS "-O2 -Wall  -Dunix -fomit-frame-pointer -funroll-loops  -std=c++11 -stdlib=libc++")
else()
  if(MINGW OR MSVC)
    if(MINGW) 
      set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops")
      set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32 -fomit-frame-pointer  -funroll-loops -static-libgcc -static-libstdc++")
     else()
      set(CMAKE_C_FLAGS "-O3 -DWIN32 -D_WIN32")
      set(CMAKE_CXX_FLAGS "-O3 -DWIN32 -D_WIN32")

     endif()
      
   else()
    set(CMAKE_C_FLAGS "-O3 -Wall -Dlinux -Dunix -fomit-frame-pointer -funroll-loops")
  endif()
endif()

# portsf - no use of sfsys here
link_directories(../lib )

include_directories(../include)

add_executable(fastconv fconv.cpp genrespframe2.cpp mxfftd.c)

target_link_libraries(fastconv portsf ${EXTRA_LIBRARIES})

my_install(fastconv)

