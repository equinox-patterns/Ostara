# VExceptionLib — Exception hierarchy (Qt::Core only)
# Private include of ../vmisc for defglobal.h (V_NOEXCEPT_EXPR macro)

set(VEXCEPTION_SOURCES
    vexception.cpp
    vexception.h
)

add_library(VExceptionLib STATIC ${VEXCEPTION_SOURCES})

target_include_directories(VExceptionLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vmisc   # defglobal.h
)

target_link_libraries(VExceptionLib PUBLIC Qt6::Core)
target_precompile_headers(VExceptionLib PRIVATE stable.h)
