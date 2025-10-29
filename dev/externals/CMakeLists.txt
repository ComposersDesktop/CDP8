add_subdirectory(portsf)
add_subdirectory(fastconv)
add_subdirectory(reverb)
add_subdirectory(mctools)
# Oct 2025: NB: these will likely be moved to separate project PVXTOOLS
# to enable Win32 CMake build using MSVC
#if(USE_LOCAL_PORTAUDIO)
#  add_subdirectory(paprogs)
#endif()
