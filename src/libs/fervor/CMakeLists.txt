# FervorLib — Software auto-updater (Sparkle-style)

set(FERVOR_SOURCES
    fvupdatewindow.cpp
    fvupdater.cpp
    fvavailableupdate.cpp
    fvupdatewindow.h
    fvupdater.h
    fvavailableupdate.h
    fvupdatewindow.ui
)

add_library(FervorLib STATIC ${FERVOR_SOURCES})

target_include_directories(FervorLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(FervorLib
    PUBLIC
        Qt6::Widgets
        Qt6::Network
        IFCLib
)

target_precompile_headers(FervorLib PRIVATE stable.h)
