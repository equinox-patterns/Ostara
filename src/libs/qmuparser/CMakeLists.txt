# QMUParserLib — Math expression parser (muParser fork)
# Always shared/dynamic (framework on macOS when not static build)
# Matches QBS VDynamicLib behavior

set(QMUPARSER_SOURCES
    qmuparser.cpp
    qmuparsertokenreader.cpp
    qmuparsererror.cpp
    qmuparsercallback.cpp
    qmuparserbytecode.cpp
    qmuparserbase.cpp
    qmuparsertest.cpp
    qmutranslation.cpp
    qmuformulabase.cpp
    qmutokenparser.cpp
    qmudef.cpp
    qmuparser.h
    qmuparser_global.h
    qmuparsertokenreader.h
    qmuparsertoken.h
    qmuparserfixes.h
    qmuparsererror.h
    qmuparserdef.h
    qmuparsercallback.h
    qmuparserbytecode.h
    qmuparserbase.h
    qmuparsertest.h
    qmutranslation.h
    qmudef.h
    qmuformulabase.h
    qmutokenparser.h
    qmuparsercallback_p.h
    make_unique.h
)

if(APPLE AND NOT OSTARA_STATIC_BUILD)
    add_library(QMUParserLib SHARED ${QMUPARSER_SOURCES})
    set_target_properties(QMUParserLib PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER "com.ostara.QMUParserLib"
        INSTALL_NAME_DIR "@rpath"
        VERSION "2.7.0"
        SOVERSION "2.7.0"
    )
else()
    add_library(QMUParserLib SHARED ${QMUPARSER_SOURCES})
    set_target_properties(QMUParserLib PROPERTIES
        VERSION "2.7.0"
        SOVERSION "2.7.0"
    )
endif()

target_include_directories(QMUParserLib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QMUParserLib PUBLIC Qt6::Core)
target_compile_definitions(QMUParserLib PRIVATE
    QMUPARSER_LIBRARY
    QMUP_VERSION="2.7.0"
)
target_precompile_headers(QMUParserLib PRIVATE stable.h)
