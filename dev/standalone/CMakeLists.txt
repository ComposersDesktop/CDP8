if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -mmacosx-version-min=10.8 -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()

link_directories(../cdp2k ../sfsys)

include_directories(../../include)

add_executable(analjoin analjoin.c)
target_link_libraries(analjoin cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(analjoin)

add_executable(brktopi brktopi.c)
target_link_libraries(brktopi cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(brktopi)

add_executable(constrict constrict.c)
target_link_libraries(constrict cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(constrict)

add_executable(dshift dshift.c)
target_link_libraries(dshift ${EXTRA_LIBRARIES})

my_install(dshift)

add_executable(envnu envnu.c)
target_link_libraries(envnu cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(envnu)

add_executable(features features.c)
target_link_libraries(features cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(features)

add_executable(flutter flutter.c)
target_link_libraries(flutter cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(flutter)

add_executable(fofex fofex.c)
target_link_libraries(fofex cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(fofex)

add_executable(frame frame.c)
target_link_libraries(frame cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(frame)

add_executable(freeze freeze.c)
target_link_libraries(freeze cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(freeze)

add_executable(gate gate.c)
target_link_libraries(gate cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(gate)

add_executable(get_partials get_partials.c)
target_link_libraries(get_partials cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(get_partials)

add_executable(grainex grainex.c)
target_link_libraries(grainex cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(grainex)

add_executable(hover hover.c)
target_link_libraries(hover cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(hover)

add_executable(manysil manysil.c)
target_link_libraries(manysil cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(manysil)

add_executable(mchanpan mchanpan.c)
target_link_libraries(mchanpan cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchanpan)

add_executable(mchanrev mchanrev.c)
target_link_libraries(mchanrev cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchanrev)

add_executable(mchiter mchiter.c)
target_link_libraries(mchiter cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchiter)

add_executable(mchshred  mchshred.c)
target_link_libraries(mchshred  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchshred )

add_executable(mchzig mchzig.c)
target_link_libraries(mchzig cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchzig)

add_executable(mchstereo mchstereo.c)
target_link_libraries(mchstereo cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mchstereo)

add_executable(mton mton.c)
target_link_libraries(mton cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(mton)

add_executable(multimix multimix.c)
target_link_libraries(multimix cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(multimix)

add_executable(newmix newmix.c)
target_link_libraries(newmix cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(newmix)

add_executable(oneform oneform.c)
target_link_libraries(oneform cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(oneform)

add_executable(peak sethares.c)
target_link_libraries(peak cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(peak)

add_executable(peakfind peakfind.c)
target_link_libraries(peakfind cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(peakfind)

add_executable(phase phase.c)
target_link_libraries(phase cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(phase)

add_executable(prefix prefix.c)
target_link_libraries(prefix cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(prefix)

add_executable(psow psow.c)
target_link_libraries(psow cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(psow)

add_executable(ptobrk ptobrk.c)
target_link_libraries(ptobrk cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(ptobrk)

add_executable(retime retime.c)
target_link_libraries(retime cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(retime)

add_executable(search search.c)
target_link_libraries(search cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(search)

add_executable(spacedesign spacedesign.c)
target_link_libraries(spacedesign cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(spacedesign)

add_executable(speclean  speclean.c)
target_link_libraries(speclean  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(speclean )

add_executable(specnu specnu.c)
target_link_libraries(specnu cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(specnu)

add_executable(specross  specross.c)
target_link_libraries(specross  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(specross )

add_executable(strans strans_multi.c)
target_link_libraries(strans cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(strans)

add_executable(texmchan texmchan.c texprepromch.c texture5mc.c
               ../texture/texperm.c ../texture/texture1.c
               ../texture/texture2.c ../texture/texture3.c ../texture/texture4.c )
target_link_libraries(texmchan cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(texmchan)

add_executable(topantail2 topantail2.c)
target_link_libraries(topantail2 cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(topantail2)

add_executable(wrappage  wrappage.c)
target_link_libraries(wrappage  cdp2k sfsys ${EXTRA_LIBRARIES})

my_install(wrappage )

 
