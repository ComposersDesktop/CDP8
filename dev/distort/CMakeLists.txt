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

add_executable(distort ap_distort.c disprepro.c distdel.c distflt.c distintlv.c distort.c distorta.c 
	distorte.c distortf.c distorth.c distortion.c distortm.c distorto.c distortp.c 
	distortr.c distorts.c distresize.c distrpl.c distrpt.c disttel.c main.c pulse.c)

target_link_libraries(distort cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(distort)

