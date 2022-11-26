# Test projects
# Use the macro to add test projects

MACRO(ADD_TEST appl_name)
   ADD_EXECUTABLE(${RELEASE}/${appl_name} "${appl_name}.c")
   TARGET_LINK_LIBRARIES(${RELEASE}/${appl_name} portaudio_static)
ENDMACRO(ADD_TEST)

ADD_TEST(patest_longsine)
