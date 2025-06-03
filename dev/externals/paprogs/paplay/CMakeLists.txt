add_executable(paplay paplay.c fmhfuncs.c)

target_include_directories(paplay PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/include
    ${CMAKE_SOURCE_DIR}/dev/externals/include
)

target_link_libraries(paplay PRIVATE
    portsf
    ${LIBCDP_LIBS}
)

my_install(paplay)