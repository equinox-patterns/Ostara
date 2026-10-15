# VThemeLib — QSS/styles/themes

set(VTHEME_SOURCES
    themeDef.h
    themeDef.cpp
    vtheme.h
    vtheme.cpp
    vapplicationstyle.h
    vapplicationstyle.cpp
    vscenestylesheet.h
    vscenestylesheet.cpp
    vstylesheetstyle.h
    vstylesheetstyle.cpp
    vtoolstyle.h
    vtoolstyle.cpp
    vpatternpiecestyle.h
    vpatternpiecestyle.cpp
)

add_library(VThemeLib STATIC ${VTHEME_SOURCES})

target_include_directories(VThemeLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vmisc   # def.h, vcommonsettings.h, compatibility.h
)

target_link_libraries(VThemeLib
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        VMiscLib
)

target_precompile_headers(VThemeLib PRIVATE stable.h)
