# VCrashHandlerLib — Crashpad integration (optional, disabled by default)

set(VCRASHHANDLER_SOURCES
    crashhandler.h
    crashhandler.cpp
    vcrashpaths.h
    vcrashpaths.cpp
)

add_library(VCrashHandlerLib STATIC ${VCRASHHANDLER_SOURCES})

target_include_directories(VCrashHandlerLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VCrashHandlerLib
    PUBLIC
        Qt6::Core
        VMiscLib
)

# Crashpad integration (when ENABLE_CRASHPAD is ON and Conan provides crashpad)
if(ENABLE_CRASHPAD)
    target_compile_definitions(VCrashHandlerLib PUBLIC CRASH_REPORTING)
    # find_package(crashpad) — requires Conan setup, for now just document
    if(APPLE)
        target_link_libraries(VCrashHandlerLib PUBLIC
            "-framework AppKit"
            "-framework Security"
            "-lbsm"
        )
    elseif(WIN32)
        target_link_libraries(VCrashHandlerLib PUBLIC Advapi32)
    elseif(UNIX)
        target_link_libraries(VCrashHandlerLib PUBLIC dl)
    endif()
endif()

target_precompile_headers(VCrashHandlerLib PRIVATE stable.h)
