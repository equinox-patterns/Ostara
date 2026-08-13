# VFormatLib — Formatting, label templates, watermarks, measurements

set(VFORMAT_SOURCES
    vdimensions.cpp
    vmeasurements.cpp
    vpatternrecipe.cpp
    vsinglelineoutlinechar.cpp
    vsinglelineoutlinechar.h
    vwatermark.cpp
    vdimensions.h
    vmeasurements.h
    vpatternrecipe.h
    vwatermark.h
)

# knownmeasurements/
set(VFORMAT_KNOWN
    knownmeasurements/vknownmeasurement.h
    knownmeasurements/vknownmeasurements.cpp
    knownmeasurements/vknownmeasurements.h
    knownmeasurements/vknownmeasurements_p.h
    knownmeasurements/vknownmeasurementsdocument.cpp
    knownmeasurements/vknownmeasurementsdocument.h
    knownmeasurements/vknownmeasurementsdatabase.cpp
    knownmeasurements/vknownmeasurementsdatabase.h
)

add_library(VFormatLib STATIC ${VFORMAT_SOURCES} ${VFORMAT_KNOWN})

target_include_directories(VFormatLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VFormatLib
    PUBLIC
        Qt6::Gui
        Qt6::Xml
        IFCLib
)

target_precompile_headers(VFormatLib PRIVATE stable.h)
