# VMiscLib — Miscellaneous utilities, settings, SVG fonts, dialogs
# The foundation lib that almost everything else depends on

set(VMISC_SOURCES
    def.cpp
    testpath.cpp
    vabstractvalapplication.cpp
    vabstractapplication.cpp
    projectversion.cpp
    vcommonsettings.cpp
    vfontinstaller.cpp
    vfontinstaller.h
    vvalentinasettings.cpp
    commandoptions.cpp
    qxtcsvmodel.cpp
    vtablesearch.cpp
    literals.cpp
    vmodifierkey.cpp
    vabstractshortcutmanager.cpp
    vabstractshortcutmanager.h
    vtranslator.cpp
    vtranslator.h
    vmainbase.h
    # Headers
    compatibility.h
    lambdaconstants.h
    def.h
    testpath.h
    vabstractvalapplication.h
    vmath.h
    vabstractapplication.h
    projectversion.h
    vcommonsettings.h
    vvalentinasettings.h
    debugbreak.h
    FactoryWidgets.h
    vlockguard.h
    vsysexits.h
    commandoptions.h
    qxtcsvmodel.h
    vtablesearch.h
    customevents.h
    defglobal.h
    testvapplication.h
    literals.h
    vmodifierkey.h
    typedef.h
)

# Dialogs
set(VMISC_DIALOGS
    dialogs/dialogaskcollectstatistic.cpp
    dialogs/dialogaskcollectstatistic.h
    dialogs/dialogaskcollectstatistic.ui
    dialogs/dialogcredits.cpp
    dialogs/dialogcredits.h
    dialogs/dialogcredits.ui
    dialogs/dialogexporttocsv.cpp
    dialogs/dialogselectlanguage.cpp
    dialogs/dialogexporttocsv.h
    dialogs/dialogselectlanguage.h
    dialogs/dialogexporttocsv.ui
    dialogs/dialogselectlanguage.ui
    dialogs/dialogselectmeasurementstype.cpp
    dialogs/dialogselectmeasurementstype.h
    dialogs/dialogselectmeasurementstype.ui
    dialogs/vshortcutdialog.cpp
    dialogs/vshortcutdialog.h
    dialogs/vshortcutdialog.ui
)

# SVG font engine
set(VMISC_SVGFONT
    svgfont/svgdef.cpp
    svgfont/svgdef.h
    svgfont/vsvgfont.cpp
    svgfont/vsvgfont.h
    svgfont/vsvgfont_p.h
    svgfont/vsvgfontdatabase.cpp
    svgfont/vsvgfontdatabase.h
    svgfont/vsvgfontengine.cpp
    svgfont/vsvgfontengine.h
    svgfont/vsvgfontengine_p.h
    svgfont/vsvgfontreader.cpp
    svgfont/vsvgfontreader.h
    svgfont/vsvgglyph.cpp
    svgfont/vsvgglyph.h
    svgfont/vsvgglyph_p.h
    svgfont/vsvghandler.cpp
    svgfont/vsvghandler.h
    svgfont/vsvgpathtokenizer.cpp
    svgfont/vsvgpathtokenizer.h
)

# Internal text codec (Qt 6, withTextCodec=false)
set(VMISC_CODECS
    vtextcodec.cpp
    vtextcodec.h
)

# QRC resources
set(VMISC_QRC
    share/resources/icon.qrc
    share/resources/flags.qrc
    share/resources/breezethemes/breeze.qrc
    share/resources/scenestyle.qrc
    share/resources/cursor.qrc
    share/resources/ostara_theme.qrc
)

# Windows-only icon themes — malgré le nom, toujours inclus (pas de condition dans le QBS)
list(APPEND VMISC_QRC
    share/resources/win_light_theme.qrc
    share/resources/win_dark_theme.qrc
)

# Note: mac_light_theme.qrc, mac_dark_theme.qrc and theme.qrc exist on disk
# but are NOT listed in vmisc.qbs — they may be used by vtheme or the app target.

# macOS native window theme (Objective-C++)
if(APPLE)
    list(APPEND VMISC_SOURCES
        macwindowtheme.h
        macwindowtheme.mm
    )
    set_source_files_properties(macwindowtheme.mm PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
    )
endif()

add_library(VMiscLib STATIC
    ${VMISC_SOURCES}
    ${VMISC_DIALOGS}
    ${VMISC_SVGFONT}
    ${VMISC_CODECS}
    ${VMISC_QRC}
)

target_include_directories(VMiscLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base — permet #include "vmisc/def.h"
)

target_link_libraries(VMiscLib
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::PrintSupport
        VExceptionLib
)

# Windows: link User32 (matches QBS staticLibraries)
if(WIN32)
    target_link_libraries(VMiscLib PUBLIC User32)
endif()

target_precompile_headers(VMiscLib PRIVATE stable.h)

# AUTOUIC: help Qt find ui_ headers from the dialogs/ subfolder
set_source_files_properties(
    ${VMISC_DIALOGS}
    PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/dialogs"
)
