add_executable(listaudevs devs.c)

target_include_directories(listaudevs PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/include
    ${CMAKE_SOURCE_DIR}/dev/externals/include
)

target_link_libraries(listaudevs PRIVATE
    ${LIBCDP_LIBS}
)

my_install(listaudevs)