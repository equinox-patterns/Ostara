# IFCLib — XML converters, schema, pattern version migration
# Depends on Xerces-C (Homebrew or Conan)

set(IFC_SOURCES
    ifcdef.h
    ifcdef.cpp
    omeasurementtypes.h
)

# exception/
set(IFC_EXCEPTION
    exception/vexceptionobjecterror.h
    exception/vexceptionemptyparameter.h
    exception/vexceptionconversionerror.h
    exception/vexceptionbadid.h
    exception/vexceptionterminatedposition.h
    exception/vexceptionwrongid.h
    exception/vexceptionundo.h
    exception/vexceptioninvalidnotch.h
    exception/vexceptioninvalidhistory.h
    exception/vexceptionobjecterror.cpp
    exception/vexceptionemptyparameter.cpp
    exception/vexceptionconversionerror.cpp
    exception/vexceptionbadid.cpp
    exception/vexceptionterminatedposition.cpp
    exception/vexceptionwrongid.cpp
    exception/vexceptionundo.cpp
    exception/vexceptioninvalidnotch.cpp
    exception/vexceptioninvalidhistory.cpp
)

# xml/
set(IFC_XML
    xml/utils.h
    xml/vabstractconverter.h
    xml/vbackgroundpatternimage.h
    xml/vdomdocument.h
    xml/vparsererrorhandler.cpp
    xml/vparsererrorhandler.h
    xml/vpatternblockmapper.cpp
    xml/vpatternblockmapper.h
    xml/vpatternconverter.h
    xml/vpatterngraph.cpp
    xml/vpatterngraph.h
    xml/vpatterngraphnode.cpp
    xml/vpatterngraphnode.h
    xml/vpatternimage.h
    xml/vtoolrecord.h
    xml/vabstractpattern.h
    xml/vvstconverter.h
    xml/vvitconverter.h
    xml/vabstractmconverter.h
    xml/vwatermarkconverter.h
    xml/utils.cpp
    xml/vabstractconverter.cpp
    xml/vbackgroundpatternimage.cpp
    xml/vdomdocument.cpp
    xml/vpatternconverter.cpp
    xml/vpatternimage.cpp
    xml/vtoolrecord.cpp
    xml/vabstractpattern.cpp
    xml/vvstconverter.cpp
    xml/vvitconverter.cpp
    xml/vabstractmconverter.cpp
    xml/vwatermarkconverter.cpp
    xml/vknownmeasurementsconverter.cpp
    xml/vknownmeasurementsconverter.h
    xml/oomtdocument.h
    xml/oomtdocument.cpp
    xml/ootudocument.h
    xml/ootudocument.cpp
    xml/oomtconverter.h
    xml/oomtconverter.cpp
)

# QRC
set(IFC_QRC
    schema.qrc
)

add_library(IFCLib STATIC
    ${IFC_SOURCES}
    ${IFC_EXCEPTION}
    ${IFC_XML}
    ${IFC_QRC}
)

target_include_directories(IFCLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(IFCLib
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Xml
        Qt6::Svg
        Qt6::Concurrent
        VMiscLib
        VThemeLib
        GraafLib
        XercesC::XercesC
)

target_precompile_headers(IFCLib PRIVATE stable.h)
