# VTestLib — les outils communs des bancs d'essai (AbstractTest, VTest).
#
# Le banc du modèle en a besoin : une trentaine de ses fichiers dérivent d'`AbstractTest`. La bibliothèque
# n'est construite que pour les essais (`-DBUILD_TESTING=ON`), et ne dépend d'aucune application — seulement
# de VGeometryLib, VLayoutLib, VPatternDBLib, VMiscLib et VExceptionLib.
#
# `TRANSLATIONS_DIR` venait des chemins d'installation fournis par qbs : il désignait les traductions
# compilées, pour un essai qui lance l'application en sous-processus. Il ne sert plus qu'à
# `AbstractTest::TranslationsPath()`, et sa valeur est vide depuis que qbs est parti.
# `VALENTINA_PATH` et `TAPE_PATH` sont partis avec leurs fonctions : `ValentinaPath()` et `TapePath()`
# n'avaient plus aucun appelant, et la seconde nommait une application qui n'existe plus.

set(VTEST_SOURCES
    abstracttest.cpp
    abstracttest.h
    vtest.cpp
    vtest.h
)

# Qt Test n'est pas demandé par la construction de l'application : cette bibliothèque ne vit que pour les
# essais, c'est donc ici qu'on le réclame.
find_package(Qt6 REQUIRED COMPONENTS Test)

add_library(VTestLib STATIC ${VTEST_SOURCES})

target_include_directories(VTestLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_definitions(VTestLib
    PRIVATE
        TRANSLATIONS_DIR=""
)

target_link_libraries(VTestLib
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
        VExceptionLib
        VGeometryLib
        VLayoutLib
        VMiscLib
        VPatternDBLib
)
