# Tape — Measurements editor app

file(GLOB TAPE_SOURCES *.cpp *.h *.ui)
file(GLOB_RECURSE TAPE_DIALOGS dialogs/*.cpp dialogs/*.h dialogs/*.ui)

set(TAPE_QRC
    share/resources/tapeicon.qrc
)

qt_add_executable(Tape
    ${TAPE_SOURCES}
    ${TAPE_DIALOGS}
    ${TAPE_QRC}
)

target_include_directories(Tape PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Tape PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    VDispatchLib
    VMiscLib
    VThemeLib
    VPatternDBLib
    FervorLib
    QMUParserLib
    VFormatLib
    VWidgetsLib
    VToolsLib
    VGAnalyticsLib
    XercesC::XercesC
    OpenSSL::SSL
    OpenSSL::Crypto
)

# macOS bundle
if(APPLE)
    set_target_properties(Tape PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ostara.tape"
        MACOSX_BUNDLE_BUNDLE_NAME "Tape"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

target_precompile_headers(Tape PRIVATE stable.h)
