
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)

if(APPLE)
	execute_process(
        COMMAND brew --prefix portaudio
        OUTPUT_VARIABLE BREW_PORTAUDIO
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(DEFINED BREW_PORTAUDIO)
		message(STATUS "BREW_PORTAUDIO Found: ${BREW_PORTAUDIO}")
		list(APPEND CMAKE_PREFIX_PATH "${BREW_PORTAUDIO}")
	endif()
endif()


add_subdirectory(listaudevs)
add_subdirectory(paplay)
add_subdirectory(pvplay)
add_subdirectory(recsf)
