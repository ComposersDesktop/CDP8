if(APPLE)
  set(CMAKE_C_FLAGS "-O -Wall -Dunix")
else()
  if(MINGW OR MSVC)
    if(MSVC)
        set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32 -D_CRT_SECURE_NO_DEPRECATE")
    else()
        set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32 -D_WIN32")
    endif()
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -DCDP97 -Dunix -Dlinux -D_X86_")
  endif()
endif()

include_directories(../newinclude)

set(PVXIO2_SRC pvfileio.c)

add_library(pvxio2 ${PVXIO2_SRC})

