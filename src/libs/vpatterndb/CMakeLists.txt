# VPatternDBLib — Pattern data model, container, pieces, formulas, calculator

set(VPATTERN_SOURCES
    oannotationdata.cpp
    oinfoblockdata.cpp
    testpassmark.cpp
    vcontainer.cpp
    calculator.cpp
    vnodedetail.cpp
    vtranslatevars.cpp
    vformula.cpp
    vpiece.cpp
    vpiecenode.cpp
    vpiecepath.cpp
    vpassmark.cpp
    oannotationdata.h
    oinfoblockdata.h
    testpassmark.h
    vcontainer.h
    calculator.h
    vnodedetail.h
    vnodedetail_p.h
    vtranslatevars.h
    vformula.h
    vpiece.h
    vpiece_p.h
    vpiecenode.h
    vpiecenode_p.h
    vpiecepath.h
    vpiecepath_p.h
    vformula_p.h
    vpassmark.h
)

set(VPATTERN_VARIABLES
    variables/vpiecearea.cpp
    variables/varcradius.cpp
    variables/vcurveangle.cpp
    variables/vcurvelength.cpp
    variables/vcurvevariable.cpp
    variables/vincrement.cpp
    variables/vinternalvariable.cpp
    variables/vlineangle.cpp
    variables/vlinelength.cpp
    variables/vmeasurement.cpp
    variables/vvariable.cpp
    variables/vcurveclength.cpp
    variables/vpiecearea.h
    variables/vpiecearea_p.h
    variables/varcradius.h
    variables/varcradius_p.h
    variables/vcurveangle.h
    variables/vcurvelength.h
    variables/vcurvevariable.h
    variables/vcurvevariable_p.h
    variables/vincrement.h
    variables/vincrement_p.h
    variables/vinternalvariable.h
    variables/vinternalvariable_p.h
    variables/vlineangle.h
    variables/vlineangle_p.h
    variables/vlinelength.h
    variables/vlinelength_p.h
    variables/vmeasurement.h
    variables/vmeasurement_p.h
    variables/vvariable.h
    variables/vvariable_p.h
    variables/vcurveclength.h
)

set(VPATTERN_FLOATITEM
    floatItemData/vgrainlinedata.cpp
    floatItemData/vabstractfloatitemdata.cpp
    floatItemData/vgrainlinedata.h
    floatItemData/vabstractfloatitemdata.h
    floatItemData/vabstractfloatitemdata_p.h
    floatItemData/vgrainlinedata_p.h
    floatItemData/floatitemdef.h
)

add_library(VPatternDBLib STATIC
    ${VPATTERN_SOURCES}
    ${VPATTERN_VARIABLES}
    ${VPATTERN_FLOATITEM}
)

target_include_directories(VPatternDBLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VPatternDBLib
    PUBLIC
        Qt6::Concurrent
        VMiscLib
        VThemeLib
        IFCLib
        VGeometryLib
        QMUParserLib
        VFormatLib
)

target_precompile_headers(VPatternDBLib PRIVATE stable.h)
