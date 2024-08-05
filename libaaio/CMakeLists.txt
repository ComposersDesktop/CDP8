set(PROJECT_NAME aaio)

set(PROJECT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/aaio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aaio1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aaio2.c
)

add_library( 
    ${PROJECT_NAME} 
    STATIC
    ${PROJECT_SOURCES}
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# target_compile_definitions(
#     ${PROJECT_NAME}
#     PUBLIC
#     -DNDEBUG
# )

# target_compile_options(
#     ${PROJECT_NAME}
#     PUBLIC
#     $<$<PLATFORM_ID:Darwin>:-Wno-unused-result>
#     $<$<PLATFORM_ID:Darwin>:-Wsign-compare>
#     $<$<PLATFORM_ID:Darwin>:-Wunreachable-code>
#     $<$<PLATFORM_ID:Darwin>:-fno-common>
#     $<$<PLATFORM_ID:Darwin>:-Wall>  
#     $<$<PLATFORM_ID:Darwin>:-g>
#     $<$<PLATFORM_ID:Darwin>:-fwrapv>
#     $<$<PLATFORM_ID:Darwin>:-O3>
#     $<$<PLATFORM_ID:Windows>:/O2>
#     $<$<PLATFORM_ID:Windows>:/MD>
# )

# target_link_options(
#     ${PROJECT_NAME}
#     PUBLIC
#     $<$<PLATFORM_ID:Darwin>:-dynamic>
# )

# target_link_directories(
#     ${PROJECT_NAME} 
#     PUBLIC
#     ${Python_LIBRARY_DIRS}
#     ${PROJECT_LIB_DIRS}
# )

# target_link_libraries(
#     ${PROJECT_NAME} 
#     PUBLIC
#     ${Python_LIBRARIES}
#     ${LLAMA_LINK_LIBS}
# )
