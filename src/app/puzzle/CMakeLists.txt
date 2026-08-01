# Puzzle — Layout/puzzle standalone app

file(GLOB PUZZLE_SOURCES *.cpp *.h *.ui)
file(GLOB_RECURSE PUZZLE_DIALOGS dialogs/*.cpp dialogs/*.h dialogs/*.ui)
file(GLOB_RECURSE PUZZLE_UNDO undocommands/*.cpp undocommands/*.h)
file(GLOB_RECURSE PUZZLE_XML xml/*.cpp xml/*.h)
file(GLOB_RECURSE PUZZLE_CAROUSEL carousel/*.cpp carousel/*.h)
file(GLOB_RECURSE PUZZLE_LAYOUT layout/*.cpp layout/*.h)
file(GLOB_RECURSE PUZZLE_SCENE scene/*.cpp scene/*.h)

set(PUZZLE_QRC
    share/resources/puzzlecursor.qrc
    share/resources/puzzleicon.qrc
)

qt_add_executable(Puzzle
    ${PUZZLE_SOURCES}
    ${PUZZLE_DIALOGS}
    ${PUZZLE_UNDO}
    ${PUZZLE_XML}
    ${PUZZLE_CAROUSEL}
    ${PUZZLE_LAYOUT}
    ${PUZZLE_SCENE}
    ${PUZZLE_QRC}
)

target_include_directories(Puzzle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Puzzle PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    Qt6::Concurrent
    VDispatchLib
    VMiscLib
    VThemeLib
    VLayoutLib
    IFCLib
    VFormatLib
    VWidgetsLib
    FervorLib
    VGAnalyticsLib
    XercesC::XercesC
    OpenSSL::SSL
    OpenSSL::Crypto
)

# macOS bundle
if(APPLE)
    set_target_properties(Puzzle PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ostara.puzzle"
        MACOSX_BUNDLE_BUNDLE_NAME "Puzzle"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

target_precompile_headers(Puzzle PRIVATE stable.h)

# Help AUTOUIC find ui_ headers
target_include_directories(Puzzle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/configpages
)
