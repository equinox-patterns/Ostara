# VWidgetsLib — Shared widgets, graphics items, main window classes

set(VWIDGETS_SOURCES
    vaspectratiopixmaplabel.cpp
    vaspectratiopixmaplabel.h
    oclickselectallfilter.cpp
    odecimalseparatorfilter.cpp
    vcomboboxdelegate.cpp
    vdecorationaligningdelegate.cpp
    velidedlabel.cpp
    vmaingraphicsscene.cpp
    vmaingraphicsview.cpp
    vmousewheelwidgetadjustmentguard.cpp
    vmousewheelwidgetadjustmentguard.h
    vsponsorcredits.cpp
    vsponsorcredits.h
    vtoolbuttonpopup.cpp
    vtoolbuttonpopup.h
    vwidgetpopup.cpp
    vcontrolpointspline.cpp
    vgraphicssimpletextitem.cpp
    vsimplepoint.cpp
    vabstractsimple.cpp
    vnobrushscalepathitem.cpp
    osamarginhighlights.cpp
    osamarginpalette.cpp
    vsimplecurve.cpp
    vabstractmainwindow.cpp
    ocartouchepainter.cpp
    ohitshapepathitem.cpp
    ogroupmembershipmark.cpp
    vgrainlineitem.cpp
    vpieceitem.cpp
    vcurvepathitem.cpp
    global.cpp
    vscenepoint.cpp
    scalesceneitems.cpp
    vlineedit.cpp
    vplaintextedit.cpp
    vhighlighter.cpp
    vpiecegrainline.cpp
    vpiecegrainline_p.h
    # Fancy tab bar
    fancytabbar/fancytab.cpp
    fancytabbar/fancytabbar.cpp
    fancytabbar/stylehelper.cpp
    # Headers
    oclickselectallfilter.h
    odecimalseparatorfilter.h
    vcomboboxdelegate.h
    vdecorationaligningdelegate.h
    velidedlabel.h
    vmaingraphicsscene.h
    vmaingraphicsview.h
    vwidgetpopup.h
    vcontrolpointspline.h
    vgraphicssimpletextitem.h
    vsimplepoint.h
    vabstractsimple.h
    vnobrushscalepathitem.h
    osamarginhighlights.h
    osamarginpalette.h
    vsimplecurve.h
    vabstractmainwindow.h
    ocartouchepainter.h
    ohitshapepathitem.h
    ogroupmembershipmark.h
    vgrainlineitem.h
    vpieceitem.h
    vcurvepathitem.h
    global.h
    vscenepoint.h
    scalesceneitems.h
    vlineedit.h
    vplaintextedit.h
    vhighlighter.h
    vpiecegrainline.h
    fancytabbar/fancytab.h
    fancytabbar/fancytabbar.h
    fancytabbar/stylehelper.h
    vsegmentlabel.cpp
    vsegmentlabel.h
)

add_library(VWidgetsLib STATIC ${VWIDGETS_SOURCES})

target_include_directories(VWidgetsLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VWidgetsLib
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Xml
        Qt6::OpenGLWidgets
        VMiscLib
        VThemeLib
        VPropertyExplorerLib
        VPatternDBLib
)

target_precompile_headers(VWidgetsLib PRIVATE stable.h)
