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

add_executable(brkdur brkdur.c)
target_link_libraries(brkdur cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(brkdur)

add_executable(diskspace diskspace.c )
target_link_libraries(diskspace cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(diskspace)

add_executable(fixgobo fixgobo.c)
target_link_libraries(fixgobo cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(fixgobo)

add_executable(gobo gobo.c)
target_link_libraries(gobo cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(gobo)

add_executable(gobosee gobosee.c)
target_link_libraries(gobosee cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(gobosee)

add_executable(histconv histconv.c)
target_link_libraries(histconv cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(histconv)

add_executable(listdate listdate.c)
target_link_libraries(listdate cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(listdate)

add_executable(logdate  logdate.c)
target_link_libraries(logdate  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(logdate )

add_executable(maxsamp2 maxsamp2.c)
target_link_libraries(maxsamp2 cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(maxsamp2)

add_executable(paudition paudition.c mxfft.c)
target_link_libraries(paudition cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(paudition)

add_executable(pdisplay pdisplay.c)
target_link_libraries(pdisplay cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(pdisplay)

add_executable(pmodify pmodify.c)
target_link_libraries(pmodify cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(pmodify)

add_executable(progmach progmach.c)
target_link_libraries(progmach cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(progmach)

add_executable(stretcha stretcha.c)
target_link_libraries(stretcha cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(stretcha)

add_executable(tkusage tkusage.c)
target_link_libraries(tkusage  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(tkusage)

add_executable(tkusage_other tkusage_other.c)
target_link_libraries(tkusage_other cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(tkusage_other)

add_executable(vuform vuform.c)
target_link_libraries(vuform cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(vuform)


