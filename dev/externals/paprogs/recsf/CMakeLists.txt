add_executable(recsf recsf.c)

target_include_directories(recsf PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/include
    ${CMAKE_SOURCE_DIR}/dev/externals/include
)

target_link_libraries(recsf PRIVATE
    portsf
    ${LIBCDP_LIBS}
)

my_install(recsf)