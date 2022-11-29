if(APPLE)
  set(CMAKE_C_FLAGS "-O2 -Wall -Dunix")
else()
  if(MINGW)
    set(CMAKE_C_FLAGS "-O2 -Wall -DWIN32")
  else()
    set(CMAKE_C_FLAGS "-O2 -Wall -Dlinux -Dunix")
  endif()
endif()
#RWD only need this for APPLE?
link_directories (/usr/local/lib)
find_library(AAIOLIB NAMES libaaio.a)

link_directories(../cdp2k ../newsfsys ../pvxio2)

include_directories(../newinclude)

add_executable(asciiget asciiget.c)
target_link_libraries(asciiget ${EXTRA_LIBRARIES})

my_install(asciiget)

add_executable(caltrain caltrain.c)
target_link_libraries(caltrain  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(caltrain)


add_executable(clicknew clicknew.c)
target_link_libraries(clicknew  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(clicknew)


add_executable(clip clip.c)
target_link_libraries(clip  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(clip)

add_executable(convert_to_midi convert_to_midi.c)
target_link_libraries(convert_to_midi ${EXTRA_LIBRARIES})

my_install(convert_to_midi)

add_executable(distcut distcut.c)
target_link_libraries(distcut  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(distcut)

add_executable(distmark distmark.c)
target_link_libraries(distmark  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(distmark)

add_executable(distmore distmore.c)
target_link_libraries(distmore  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(distmore)

add_executable(distrep distrep.c)
target_link_libraries(distrep  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(distrep)

add_executable(distshift distshift.c)
target_link_libraries(distshift  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(distshift)

# OUT
# add_executable(distwarp distwarp.c)
# target_link_libraries(distwarp  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
# my_install(distwarp)

add_executable(envcut envcut.c)
target_link_libraries(envcut  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(envcut)

# add_executable(envnu_other envnu_other.c)
# target_link_libraries(envnu_other  cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
# my_install(envnu_other)

add_executable(envspeak envspeak.c)
target_link_libraries(envspeak cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(envspeak)

#waiting for code fix from TW
#add_executable(extspeak extspeak.c)
#target_link_libraries(extspeak cdp2k sfsys pvxio2 ${AAIOLIB} ${EXTRA_LIBRARIES})
#my_install(extspeak)

add_executable(fractal fractal.c)
target_link_libraries(fractal cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(fractal)

# OUT
#add_executable(fturanal fturanal.c)
#target_link_libraries(fturanal cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
#my_install(fturanal)

# headview.c ?

add_executable(hover2 hover2.c)
target_link_libraries(hover2 cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(hover2)

# OUT (too many problems, undeclared variables)
# add_executable(integrate integrate.c)
# target_link_libraries(integrate ${EXTRA_LIBRARIES})
# my_install(integrate)

add_executable(onset onset.c)
target_link_libraries(onset cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(onset)

add_executable(pairex pairex.c)
target_link_libraries(pairex cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(pairex)

add_executable(partition partition.c)
target_link_libraries(partition cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(partition)

add_executable(phasor phasor.c)
target_link_libraries(phasor cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(phasor)

add_executable(quirk quirk.c)
target_link_libraries(quirk cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(quirk)

add_executable(repair repair.c)
target_link_libraries(repair cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(repair)

#add_executable(rotate rotate.c)
#target_link_libraries(rotate ${EXTRA_LIBRARIES})
#my_install(rotate)

# scales.c ignore: just textual exploratory comments

add_executable(specenv specenv.c)
target_link_libraries(specenv cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(specenv)

add_executable(speculate speculation.c)
target_link_libraries(speculate cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(speculate)

add_executable(spike spike.c)
target_link_libraries(spike cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(spike)

# OUT (ID 'SUPERLOOP' not found)
# add_executable(superloop superloop.c)
# target_link_libraries(superloop cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
# my_install(superloop)

add_executable(suppress suppress.c)
target_link_libraries(suppress cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(suppress)

# excluded by TW (standalone.h)
# add_executable(toptail toptail.c)
# target_link_libraries(toptail cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})
# my_install(toptail)

add_executable(tostereo tostereo.c)
target_link_libraries(tostereo cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(tostereo)

add_executable(matrix unitary_matrix.c)
target_link_libraries(matrix cdp2k sfsys pvxio2 ${EXTRA_LIBRARIES})

my_install(matrix)




