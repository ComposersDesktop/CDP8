if(APPLE)
  set(CMAKE_C_FLAGS "-O -Wall  -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -DCDP97 -Dunix -Dlinux -D_X86_")
  endif()
endif()

set(CDP2K_SRCS ../cdparse/cdparse.c dzsetup.c formantsg.c mainfuncs.c parstruct.c readdata.c readfiles.c special.c tkinput.c tklib1.c tklib3.c validate.c writedata.c)

add_library(cdp2k ${CDP2K_SRCS})
