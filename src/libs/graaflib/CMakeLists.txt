# GraafLib — Header-only graph algorithm library (no Qt dependency)

add_library(GraafLib INTERFACE)

target_include_directories(GraafLib
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
