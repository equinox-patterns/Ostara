# VObjLib — Wavefront OBJ export

set(VOBJ_SOURCES
    vobjengine.cpp
    vobjpaintdevice.cpp
    delaunay.cpp
    predicates.cpp
    vobjengine.h
    delaunay.h
    vobjpaintdevice.h
)

add_library(VObjLib STATIC ${VOBJ_SOURCES})

target_include_directories(VObjLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VObjLib
    PUBLIC
        Qt6::Gui
        VMiscLib
        VThemeLib
)

target_precompile_headers(VObjLib PRIVATE stable.h)
