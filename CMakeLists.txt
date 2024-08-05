# RWD version for newsfsys with PVOC EX support
cmake_minimum_required(VERSION 2.8.3)

project (CDP)

# ENABLE_TESTING()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

message(STATUS "${CMAKE_HOME_DIRECTORY}")
message(STATUS "installing to ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++11 -stdlib=libc++")
set(CMAKE_C_FLAGS_RELEASE "-O3 ")
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Project definitions
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/NewRelease")
message(STATUS "building to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Relative install paths
set(EXECUTABLE_INSTALL_DIR "bin")
function(my_install name)
  install(TARGETS ${name}
	RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}" )
endfunction(my_install)


set(LIBRARY_INSTALL_DIR "lib")
message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CMakeParseArguments)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# If FAIL_MISSING is true and the dependencies are not fulfilled,
# it will abort the cmake run.
# If FAIL_MISSING is false, it will set the option to OFF.
# If the target is not enabled, it will do nothing.
# example: check_deps(BUILD_NEW_PARSER FLEX_EXECUTABLE BISON_EXECUTABLE)
function(check_deps option)
    if(${option})
        set(i 1)
        while( ${i} LESS ${ARGC} )
            set(dep ${ARGV${i}})
            if(NOT ${dep})
                if(FAIL_MISSING)
                    message(FATAL_ERROR
                        "${option} is enabled, but ${dep}=\"${${dep}}\"")
                else()
                    message(STATUS "${dep}=\"${${dep}}\", so disabling ${option}")
                    set(${option} OFF PARENT_SCOPE)
                    # Set it in the local scope too
                    set(${option} OFF)
                endif()
            endif()
            math(EXPR i "${i}+1")
        endwhile()
    endif()
    if(${option})
        message(STATUS "${option} is enabled")
    else()
        message(STATUS "${option} is disabled")
    endif()
endfunction(check_deps)

### COMPILER OPTIMIZATION FLAGS
option(USE_COMPILER_OPTIMIZATIONS "Use the default CDP compiler optimization flags" ON)
if(USE_COMPILER_OPTIMIZATIONS)
  include(cmake/CompilerOptimizations.cmake)
endif()

if(APPLE)
  set(OSX_VERSION " ")
endif()

# Include this after the install path definitions so we can override them here.
# Also after function definitions so we can use them there
find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_HOME_DIRECTORY})
if(CUSTOM_CMAKE)
    message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
    include(${CUSTOM_CMAKE})
else()
    message(STATUS "Not using Custom.cmake file.")
endif()


if(WIN32 AND NOT MSVC)
    if(EXISTS "C:/MinGW/include")
        include_directories(C:/MinGW/include)
    else()
        MESSAGE(STATUS "MinGW include dir not found")
    endif()
    set(CMAKE_MODULE_LINKER_FLAGS
        "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()

if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX "")
    set(EXTRA_LIBRARIES
        kernel32
        user32
        ole32
        winspool
        odbc32
        gdi32
        comdlg32
        advapi32
        shell32
        oleaut32
        uuid)
    else()
      set(EXTRA_LIBRARIES "m")
endif(WIN32)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX YES)
else()
    set(LINUX NO)
endif()

## USER OPTIONS ##

option(FAIL_MISSING "Fail when a required external dependency is not present (useful for packagers)" OFF)

option(BUILD_RELEASE "Build for release" ON)

# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)

##

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)
if (HAS_VISIBILITY_HIDDEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
if (HAS_CXX_VISBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

if(APPLE)
  if(BUILD_INSTALLER)
    set(CS_FRAMEWORK_DEST "${CMAKE_INSTALL_PREFIX}")
  endif()

  set(DEFAULT_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/)

  if(NOT CMAKE_OSX_SYSROOT AND EXISTS ${DEFAULT_OSX_SYSROOT})
    set(CMAKE_OSX_SYSROOT ${DEFAULT_OSX_SYSROOT})
  endif()

   if(EXISTS ${CMAKE_OSX_SYSROOT})
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
  endif()

endif()

set(CDPLIB "lib")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-Wno-format")
#    add_compiler_flags("-g")
endif()

set(BUILD_BIN_DIR ${BUILD_DIR/bin})
set(BUILD_LIB_DIR ${BUILD_DIR}/lib)

message(STATUS "BUILD_BIN_DIR set to ${BUILD_BIN_DIR}")
message(STATUS "BUILD_LIB_DIR set to ${BUILD_LIB_DIR}")

# OS specific checks

TEST_BIG_ENDIAN(BIG_ENDIAN)

## CONFIGURATION ##

SET(BUILD_SHARED_LIBS OFF)

## HEADER/LIBRARY/OTHER CHECKS ##

option(USE_LOCAL_PORTAUDIO "Build and use PA programs" ON)

if(USE_LOCAL_PORTAUDIO)
  message(STATUS "paprogs to be built")
else()
  message(STATUS "paprogs not built")
endif()
  
set(HEADERS_TO_CHECK
    unistd.h io.h fcntl.h stdint.h
    sys/time.h sys/types.h termios.h
    values.h winsock.h sys/socket.h
    dirent.h )

foreach(header ${HEADERS_TO_CHECK})
    # Convert to uppercase and replace [./] with _
    string(TOUPPER ${header} tmp)

    string(REGEX REPLACE [./] "_" upper_header ${tmp})
    check_include_file(${header} HAVE_${upper_header})
endforeach()

##set(LIBCDP_CFLAGS -D__BUILDING_LIBCDP)

include_directories(./dev/include)
#RWD for PVOCEX support
include_directories(./dev/newinclude)

#if(WIN32)
    include_directories(${PORTAUDIO_INCLUDE_DIRECTORY})
#endif(WIN32)


include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add the install target
###??


set(LIBCDP_LIBS
    ${PORTAUDIO_LIBRARY})

if(WIN32)
    list(APPEND LIBCDP_LIBS "${CDP_WINDOWS_LIBRARIES}")
endif()

if(LINUX)
    message(STATUS "Building on Linux")
    add_definitions( -Dunix -Dlinux -D_X86_  -DFILE_AMB_SUPPORT)
    list(APPEND LIBCDP_LIBS m)
endif()

if(APPLE)
    message(STATUS "Building on OSX")
# we - CDP - need the macosx directive to distribute; not wanted for a personal build
# e.g. -mmacosx-version-min=10.8
    add_definitions( -Dunix)
    list(APPEND LIBCDP_LIBS m)
endif()

if(WIN32)
    message(STATUS "Building on Windows")
    add_definitions(-DWIN32 -D_WIN32)
endif()

#add_compiler_flags(${LIBCDP_CFLAGS} TARGETS ${CDPLIB})

add_subdirectory(libaaio)
add_subdirectory(dev)

# # uninstall target
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
#     IMMEDIATE @ONLY)

# add_custom_target(uninstall
#     COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

## target etags/tags
if(UNIX)
  add_custom_target(tags COMMAND etags `find ${CMAKE_CURRENT_SOURCE_DIR} -name \\*.cc -or -name \\*.hh -or -name \\*.cpp -or -name \\*.h -or -name \\*.c | grep -v " " `)
  add_custom_target(etags DEPENDS tags)
endif()

# build a CPack driven installer package
#include (InstallRequiredSystemLibraries)

set (CPACK_PACKAGE_NAME "cdp")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION "8.0.1")
set (CPACK_PACKAGE_VERSION_MAJOR "8")
set (CPACK_PACKAGE_VERSION_MINOR "0")
set (CPACK_PACKAGE_VERSION_PATCH "1")
include (CPack)

