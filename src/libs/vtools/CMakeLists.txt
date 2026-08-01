# VToolsLib — All drawing tools, undo commands, visualization
# Largest lib (~485 source files)
# NOTE: Uses file(GLOB) because manual enumeration of 485 files is unmaintainable.
# The QBS file (vtools.qbs) is the source of truth for which files belong here.
# CMake re-configure picks up new/removed files automatically.

file(GLOB VTOOLS_MAIN
    *.cpp *.h
)

file(GLOB VTOOLS_DIALOGS
    dialogs/*.cpp dialogs/*.h dialogs/*.ui
    dialogs/support/*.cpp dialogs/support/*.h dialogs/support/*.ui
    dialogs/tools/*.cpp dialogs/tools/*.h dialogs/tools/*.ui
    dialogs/tools/piece/*.cpp dialogs/tools/piece/*.h dialogs/tools/piece/*.ui
    dialogs/tools/piece/tabs/*.ui
    dialogs/widgets/*.cpp dialogs/widgets/*.h
)

file(GLOB VTOOLS_TOOLS
    tools/*.cpp tools/*.h
    tools/backgroundimage/*.cpp tools/backgroundimage/*.h
    tools/drawTools/*.cpp tools/drawTools/*.h
    tools/drawTools/operation/*.cpp tools/drawTools/operation/*.h
    tools/drawTools/operation/flipping/*.cpp tools/drawTools/operation/flipping/*.h
    tools/drawTools/toolcurve/*.cpp tools/drawTools/toolcurve/*.h
    tools/drawTools/toolpoint/*.cpp tools/drawTools/toolpoint/*.h
    tools/drawTools/toolpoint/tooldoublepoint/*.cpp tools/drawTools/toolpoint/tooldoublepoint/*.h
    tools/drawTools/toolpoint/toolsinglepoint/*.cpp tools/drawTools/toolpoint/toolsinglepoint/*.h
    tools/drawTools/toolpoint/toolsinglepoint/toolcut/*.cpp tools/drawTools/toolpoint/toolsinglepoint/toolcut/*.h
    tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/*.cpp tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/*.h
    tools/nodeDetails/*.cpp tools/nodeDetails/*.h
)

file(GLOB VTOOLS_UNDO
    undocommands/*.cpp undocommands/*.h
    undocommands/image/*.cpp undocommands/image/*.h
    undocommands/label/*.cpp undocommands/label/*.h
)

file(GLOB VTOOLS_VIS
    visualization/*.cpp visualization/*.h
    visualization/line/*.cpp visualization/line/*.h
    visualization/line/operation/*.cpp visualization/line/operation/*.h
    visualization/path/*.cpp visualization/path/*.h
)

add_library(VToolsLib STATIC
    ${VTOOLS_MAIN}
    ${VTOOLS_DIALOGS}
    ${VTOOLS_TOOLS}
    ${VTOOLS_UNDO}
    ${VTOOLS_VIS}
)

target_include_directories(VToolsLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VToolsLib
    PUBLIC
        Qt6::Widgets
        Qt6::Xml
        Qt6::Svg
        Qt6::Concurrent
        IFCLib
        VLayoutLib
        VFormatLib
        VWidgetsLib
        VThemeLib
)

target_precompile_headers(VToolsLib PRIVATE stable.h)

# Help AUTOUIC find ui_ headers across the deep dialog hierarchy
target_include_directories(VToolsLib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/tools/piece
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/tools/piece/tabs
)

# Tell AUTOUIC where to search for .ui files
set_target_properties(VToolsLib PROPERTIES
    AUTOUIC_SEARCH_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/dialogs/tools/piece/tabs"
)
