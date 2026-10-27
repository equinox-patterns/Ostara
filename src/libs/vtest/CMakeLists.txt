# VTestLib — les outils communs des bancs d'essai (AbstractTest, VTest).
#
# Le banc du modèle en a besoin : une trentaine de ses fichiers dérivent d'`AbstractTest`. La bibliothèque
# n'est construite que pour les essais (`-DBUILD_TESTING=ON`), et ne dépend d'aucune application — seulement
# de VGeometryLib, VLayoutLib, VPatternDBLib, VMiscLib et VExceptionLib.
#
# `VALENTINA_PATH`, `TAPE_PATH` et `TRANSLATIONS_DIR` venaient des chemins d'installation fournis par qbs :
# ils désignaient les binaires et les traductions compilés, pour les essais qui lancent l'application en
# sous-processus. Aucun essai du modèle ne s'en sert — les valeurs sont vides, et un essai qui les
# demanderait un jour devra les recevoir à la construction, pas par une constante muette.

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
        VALENTINA_PATH=""
        TAPE_PATH=""
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
