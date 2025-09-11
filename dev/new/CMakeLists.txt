if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
  link_directories (/usr/local/lib)
#  find_library(AAIOLIB NAMES libaaio.a)
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
    link_directories (/usr/local/lib)
#    find_library(AAIOLIB NAMES libaaio.a)
  endif()
endif()

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude /usr/local/include)

add_executable(cantor cantor.c)
target_link_libraries(cantor cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(cantor)

add_executable(ceracu ceracu.c)
target_link_libraries(ceracu cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(ceracu)

add_executable(chanphase chanphase.c)
target_link_libraries(chanphase cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(chanphase)

add_executable(sfecho echo.c)
target_link_libraries(sfecho cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(sfecho)

add_executable(filtrage filtrage.c)
target_link_libraries(filtrage cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(filtrage)

add_executable(fracture fracture.c)
if(MINGW)
target_link_libraries(fracture cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
else()
target_link_libraries(fracture cdp2k sfsys pvxio2 aaio ${EXTRA_LIBRARIES})
endif()
my_install(fracture)

add_executable(glisten glisten.c)
target_link_libraries(glisten cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(glisten)

add_executable(isolate isolate.c)
target_link_libraries(isolate cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(isolate)

add_executable(iterfof iterfof.c)
if(MINGW)
target_link_libraries(iterfof cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
else()
target_link_libraries(iterfof cdp2k sfsys pvxio2 aaio ${EXTRA_LIBRARIES})
endif()
my_install(iterfof)

add_executable(iterline iterline.c)
target_link_libraries(iterline cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(iterline)

add_executable(iterlinef iterlinef.c)
target_link_libraries(iterlinef cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(iterlinef)

add_executable(madrid madrid.c)
target_link_libraries(madrid cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(madrid)

add_executable(newdelay newdelay.c)
target_link_libraries(newdelay cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(newdelay)

add_executable(newmorph newmorph.c)
target_link_libraries(newmorph cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(newmorph)

add_executable(newtex newtex.c)
target_link_libraries(newtex cdp2k sfsys pvxio2 aaio ${EXTRA_LIBRARIES})

my_install(newtex)

add_executable(packet packet.c)
target_link_libraries(packet cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(packet)

add_executable(panorama panorama.c)
target_link_libraries(panorama cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(panorama)

add_executable(rejoin rejoin.c)
target_link_libraries(rejoin cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(rejoin)

add_executable(selfsim selfsim.c)
target_link_libraries(selfsim cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(selfsim)

add_executable(shifter shifter.c)
target_link_libraries(shifter cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(shifter)

add_executable(shrink shrink.c)
target_link_libraries(shrink cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(shrink)

add_executable(silend silend.c)
target_link_libraries(silend cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(silend)

add_executable(specgrids specgrids.c)
target_link_libraries(specgrids cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(specgrids)

add_executable(specsphinx specsphinx.c)
target_link_libraries(specsphinx cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(specsphinx)

add_executable(spectrum spectrum.c)
target_link_libraries(spectrum cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(spectrum)

add_executable(spectwin spectwin.c)
target_link_libraries(spectwin cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(spectwin)

add_executable(specvu specvu.c)
target_link_libraries(specvu cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(specvu)

add_executable(subtract subtract.c)
target_link_libraries(subtract cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(subtract)

add_executable(superaccu superaccu.c)
target_link_libraries(superaccu cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(superaccu)

add_executable(tangent tangent.c)
target_link_libraries(tangent cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(tangent)

add_executable(transit transit.c)
target_link_libraries(transit cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(transit)

add_executable(tremolo tremolo.c)
target_link_libraries(tremolo cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(tremolo)

add_executable(tunevary tunevary.c)
target_link_libraries(tunevary cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(tunevary)

