# VBackportLib — Qt version backport shims
# On Qt >= 6.9, this lib is empty (no backports needed)
# On Qt < 6.9, it provides qpainterstateguard

add_library(VBackportLib INTERFACE)

target_include_directories(VBackportLib
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VBackportLib INTERFACE Qt6::Gui Qt6::Core)
