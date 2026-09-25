# VLayoutLib — Layout/nesting engine, dialogs

set(VLAYOUT_SOURCES
    vlayoutexporter.cpp
    vlayoutgenerator.cpp
    vlayoutpaper.cpp
    vbank.cpp
    vcontour.cpp
    vbestsquare.cpp
    vlayoutpoint.cpp
    vposition.cpp
    vrawlayout.cpp
    vprintlayout.cpp
    vtextmanager.cpp
    otextwrap.cpp
    vposter.cpp
    vgraphicsfillitem.cpp
    vabstractpiece.cpp
    vlayoutpiece.cpp
    vlayoutpiecepath.cpp
    vrawsapoint.cpp
    vboundary.cpp
    vboundary.h
    vfoldline.h
    vfoldline.cpp
    vlayoutexporter.h
    vlayoutgenerator.h
    vlayoutdef.h
    vlayoutpaper.h
    vlayoutpaper_p.h
    vbank.h
    vcontour.h
    vcontour_p.h
    vbestsquare.h
    vlayoutpoint.h
    vposition.h
    vrawlayout.h
    vprintlayout.h
    vsapoint.h
    vtextmanager.h
    otextwrap.h
    vposter.h
    vgraphicsfillitem.h
    vabstractpiece.h
    vabstractpiece_p.h
    vlayoutpiece.h
    vlayoutpiece_p.h
    vlayoutpiecepath.h
    vlayoutpiecepath_p.h
    vbestsquare_p.h
    vrawsapoint.h
)

set(VLAYOUT_DIALOGS
    dialogs/dialoglayoutscale.h
    dialogs/vabstractlayoutdialog.h
    dialogs/watermarkwindow.h
    dialogs/dialoglayoutscale.cpp
    dialogs/vabstractlayoutdialog.cpp
    dialogs/watermarkwindow.cpp
    dialogs/dialoglayoutscale.ui
    dialogs/watermarkwindow.ui
)

set(VLAYOUT_QRC
    share/icons.qrc
)

add_library(VLayoutLib STATIC ${VLAYOUT_SOURCES} ${VLAYOUT_DIALOGS} ${VLAYOUT_QRC})

target_include_directories(VLayoutLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VLayoutLib
    PUBLIC
        Qt6::Concurrent
        Qt6::Svg
        VMiscLib
        VThemeLib
        VGeometryLib
        VPatternDBLib
        VObjLib
        QMUParserLib
        VDXFLib
        VHPGLLib
        IFCLib
        VWidgetsLib
        VFormatLib
)

target_precompile_headers(VLayoutLib PRIVATE stable.h)
