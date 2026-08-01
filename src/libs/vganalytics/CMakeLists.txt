# VGAnalyticsLib — Sanctions compliance (GeoIP country check only)
# Google Analytics tracking removed (Ostara fork).

set(VGANALYTICS_SOURCES
    vganalytics.cpp
    vganalytics.h
)

add_library(VGAnalyticsLib STATIC ${VGANALYTICS_SOURCES})

target_include_directories(VGAnalyticsLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VGAnalyticsLib
    PUBLIC
        Qt6::Core
        Qt6::Network
)
