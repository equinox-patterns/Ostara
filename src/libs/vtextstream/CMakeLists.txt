# VTextStreamLib — Text stream wrappers (Qt::Core only)

set(VTEXTSTREAM_SOURCES
    vtextstream.cpp
    vtextstream.h
    vtextstream_p.h
)

add_library(VTextStreamLib STATIC ${VTEXTSTREAM_SOURCES})

target_include_directories(VTextStreamLib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VTextStreamLib PUBLIC Qt6::Core)
target_precompile_headers(VTextStreamLib PRIVATE stable.h)
