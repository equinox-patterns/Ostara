# VDispatchLib — Qt dispatch shim (header-only but needs MOC for MsgHandlerWorkerClass)

add_library(VDispatchLib STATIC
    qt_dispatch.h
)

target_include_directories(VDispatchLib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VDispatchLib PUBLIC Qt6::Core)
