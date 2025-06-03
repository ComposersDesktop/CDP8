cmake_minimum_required(VERSION 3.16)

project(CDP)

# -----------------------------
# EXTERNAL: libaaio
# -----------------------------

set(LIBAAIO_TAR "${CMAKE_SOURCE_DIR}/libaaio/libaaio-0.3.1.tar.bz2")
set(LIBAAIO_DIR "${CMAKE_SOURCE_DIR}/libaaio/libaaio-0.3.1")

if(NOT EXISTS "${LIBAAIO_DIR}/CMakeLists.txt")
  message(STATUS "Descomprimiendo libaaio-0.3.1.tar.bz2...")
  execute_process(
    COMMAND tar -xjf "${LIBAAIO_TAR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/libaaio"
    RESULT_VARIABLE extract_result
    OUTPUT_VARIABLE extract_output
    ERROR_VARIABLE extract_error
  )
  if(NOT extract_result EQUAL 0)
    message(FATAL_ERROR "Error al descomprimir libaaio: ${extract_error}")
  endif()
else()
  message(STATUS "libaaio-0.3.1 ya descomprimido")
endif()

add_subdirectory(libaaio)
set(AAIOLIB aaio)

# -----------------------------
# CONFIGURACIONES BÁSICAS
# -----------------------------

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

message(STATUS "CMake Home Directory: ${CMAKE_HOME_DIRECTORY}")
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++11 -stdlib=libc++")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")
message(STATUS "Building to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(EXECUTABLE_INSTALL_DIR "bin")
function(my_install name)
  install(TARGETS ${name}
    RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}" )
endfunction()

set(LIBRARY_INSTALL_DIR "lib")
message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

# -----------------------------
# DEPENDENCIAS Y CHECKS
# -----------------------------

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CMakeParseArguments)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(check_deps option)
  if(${option})
    set(i 1)
    while(${i} LESS ${ARGC})
      set(dep ${ARGV${i}})
      if(NOT ${dep})
        if(FAIL_MISSING)
          message(FATAL_ERROR "${option} is enabled, but ${dep}=\"${${dep}}\"")
        else()
          message(STATUS "${dep}=\"${${dep}}\", so disabling ${option}")
          set(${option} OFF PARENT_SCOPE)
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
endfunction()

option(USE_COMPILER_OPTIMIZATIONS "Use the default CDP compiler optimization flags" ON)
if(USE_COMPILER_OPTIMIZATIONS)
  include(cmake/CompilerOptimizations.cmake)
endif()

if(APPLE)
  set(OSX_VERSION " ")
  set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY})
if(CUSTOM_CMAKE)
  message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
  include(${CUSTOM_CMAKE})
else()
  message(STATUS "Not using Custom.cmake file.")
endif()

# -----------------------------
# FLAGS ADICIONALES
# -----------------------------

check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)
if(HAS_VISIBILITY_HIDDEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
if(HAS_CXX_VISIBILITY_HIDDEN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wno-format")
endif()

# -----------------------------
# DIRECTORIOS DE INCLUDES
# -----------------------------

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(./dev/include)
include_directories(./dev/newinclude)

# -----------------------------
# FLAGS POR PLATAFORMA
# -----------------------------

if(WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
  set(CMAKE_SHARED_MODULE_PREFIX "")
  set(EXTRA_LIBRARIES
    kernel32 user32 ole32 winspool odbc32 gdi32
    comdlg32 advapi32 shell32 oleaut32 uuid)
else()
  set(EXTRA_LIBRARIES m)
endif()

if(WIN32 AND NOT MSVC)
  if(EXISTS "C:/MinGW/include")
    include_directories(C:/MinGW/include)
  else()
    MESSAGE(STATUS "MinGW include dir not found at C:/MinGW/include")
  endif()
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(LINUX YES)
else()
  set(LINUX NO)
endif()

option(FAIL_MISSING "Fail when a required external dependency is not present" OFF)
option(BUILD_RELEASE "Build for release" ON)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(BUILD_BIN_DIR ${BUILD_DIR}/bin)
set(BUILD_LIB_DIR ${BUILD_DIR}/lib)

message(STATUS "BUILD_BIN_DIR set to ${BUILD_BIN_DIR}")
message(STATUS "BUILD_LIB_DIR set to ${BUILD_LIB_DIR}")

TEST_BIG_ENDIAN(BIG_ENDIAN)
SET(BUILD_SHARED_LIBS OFF)

# -----------------------------
# PORTAUDIO (OPCIONAL)
# -----------------------------
# Activa USE_LOCAL_PORTAUDIO si necesitas compilar los programas que usan PortAudio.
# Requiere tener PortAudio instalado en el sistema (ej: brew install portaudio en macOS).
# Si está OFF, se omiten todos los programas relacionados con PortAudio.

option(USE_LOCAL_PORTAUDIO "Build and use PA programs" ON)

if(USE_LOCAL_PORTAUDIO)
  message(STATUS "USE_LOCAL_PORTAUDIO está ON: Se construirán los paprogs y se buscará PortAudio.")

  include_directories(${CMAKE_SOURCE_DIR}/externals/portaudio)

  find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
    PATHS /opt/homebrew/include /usr/local/include /usr/include
    NO_DEFAULT_PATH
  )

  find_library(PORTAUDIO_LIBRARY
    NAMES portaudio
    PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
    NO_DEFAULT_PATH
  )

  if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
    message(STATUS "PortAudio encontrado para paprogs en: ${PORTAUDIO_INCLUDE_DIR}")
    include_directories(${PORTAUDIO_INCLUDE_DIR})

    add_library(portaudio_ringbuffer STATIC
      ${CMAKE_SOURCE_DIR}/externals/portaudio/pa_ringbuffer.c
    )
    target_include_directories(portaudio_ringbuffer PUBLIC
      ${CMAKE_SOURCE_DIR}/externals/portaudio
    )
  else()
    message(FATAL_ERROR "USE_LOCAL_PORTAUDIO está ON pero no se encontró PortAudio. Por favor, instálalo (ej: brew install portaudio) o desactiva la opción USE_LOCAL_PORTAUDIO.")
  endif()
else()
  message(STATUS "USE_LOCAL_PORTAUDIO está OFF: No se construirán los paprogs, se omite la búsqueda de PortAudio.")
  set(PORTAUDIO_INCLUDE_DIR "")
  set(PORTAUDIO_LIBRARY "")
endif()

# -----------------------------
# LIBRERÍAS CDP
# -----------------------------

set(LIBCDP_LIBS)

if(AAIOLIB AND TARGET ${AAIOLIB})
  list(APPEND LIBCDP_LIBS ${AAIOLIB})
endif()

if(USE_LOCAL_PORTAUDIO)
  if(PORTAUDIO_LIBRARY)
    list(APPEND LIBCDP_LIBS ${PORTAUDIO_LIBRARY})
  endif()
  if(TARGET portaudio_ringbuffer)
    list(APPEND LIBCDP_LIBS portaudio_ringbuffer)
  endif()
endif()

if(WIN32)
  if(CDP_WINDOWS_LIBRARIES)
    list(APPEND LIBCDP_LIBS ${CDP_WINDOWS_LIBRARIES})
  endif()
endif()

if(LINUX)
  message(STATUS "Building on Linux")
  add_definitions(-Dunix -Dlinux -D_X86_ -DFILE_AMB_SUPPORT)
  list(APPEND LIBCDP_LIBS m)
endif()

if(APPLE)
  message(STATUS "Building on macOS")
  add_definitions(-Dunix)

  list(APPEND LIBCDP_LIBS
    m
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
    "-framework CoreServices"
  )
endif()

if(WIN32)
  message(STATUS "Building on Windows")
  add_definitions(-DWIN32 -D_WIN32)
endif()

# -----------------------------
# SUBDIRECTORIOS
# -----------------------------

add_subdirectory(dev)

# -----------------------------
# PACKAGING
# -----------------------------

set(CPACK_PACKAGE_NAME "cdp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION "8.0.1")
set(CPACK_PACKAGE_VERSION_MAJOR "8")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
include(CPack)