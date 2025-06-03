add_executable(pvplay
    pvplay.cpp
    pvoc2.cpp
    fmhfuncs.c
    pvthreads.cpp
    mxfft.c
)

target_include_directories(pvplay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dev/newinclude
    ${CMAKE_SOURCE_DIR}/dev/externals/portaudio/include
)

# Vincular con bibliotecas internas de CDP
target_link_libraries(pvplay
    sfsys
    pvxio2
    ${LIBCDP_LIBS}
)

my_install(pvplay)