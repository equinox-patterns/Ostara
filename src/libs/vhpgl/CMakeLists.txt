# VHPGLLib — HPGL plotter output engine

set(VHPGL_SOURCES
    vhpglengine.cpp
    vhpglpaintdevice.cpp
    vhpglengine.h
    vhpglpaintdevice.h
    stable.h
)

add_library(VHPGLLib STATIC ${VHPGL_SOURCES})

target_include_directories(VHPGLLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VHPGLLib
    PUBLIC
        Qt6::Core
        VMiscLib
        VFormatLib
)

target_precompile_headers(VHPGLLib PRIVATE stable.h)
