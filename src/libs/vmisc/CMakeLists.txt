# VMiscLib — Miscellaneous utilities, settings, SVG fonts, dialogs
# The foundation lib that almost everything else depends on

set(VMISC_SOURCES
    def.cpp
    testpath.cpp
    vabstractvalapplication.cpp
    vabstractapplication.cpp
    projectversion.cpp
    vcommonsettings.cpp
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
# Internal text codec (Qt 6, withTextCodec=false)
set(VMISC_CODECS
    vtextcodec.cpp
    vtextcodec.h
)

# QRC resources
# Les ressources de VMisc vivent dans share/, à la racine du dépôt, avec les autres données de
# l'application. Les .qrc y sont rangés à côté d'elles.
set(VMISC_QRC
    ${CMAKE_SOURCE_DIR}/share/flags.qrc
    ${CMAKE_SOURCE_DIR}/share/scenestyle.qrc
    ${CMAKE_SOURCE_DIR}/share/cursor.qrc
    ${CMAKE_SOURCE_DIR}/share/ostara_theme.qrc
)

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
