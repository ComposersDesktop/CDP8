if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -mmacosx-version-min=10.5 -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../sfsys)

include_directories(../../include)

add_executable(columns columns0.c columns1.c columns2.c columns3.c
                       columns4.c columns5.c columns6.c)
target_link_libraries(columns cdp2k sfsys ${EXTRA_LIBRARIES})
my_install(columns)

add_executable(vectors vectors.c)
target_link_libraries(vectors cdp2k sfsys ${EXTRA_LIBRARIES})
my_install(vectors)


add_executable(getcol getcol.c)
target_link_libraries(getcol cdp2k sfsys ${EXTRA_LIBRARIES})
my_install(getcol)


add_executable(putcol putcol.c)
target_link_libraries(putcol cdp2k sfsys ${EXTRA_LIBRARIES})
my_install(putcol)
