cmake_minimum_required(VERSION 3.29)
project(Ostara VERSION 1.1.0 LANGUAGES CXX)

# ═══════════════════════════════════════════════════════════════════════════════
# Options
# ═══════════════════════════════════════════════════════════════════════════════
option(BUILD_TESTING "Build unit tests" OFF)
option(ENABLE_CRASHPAD "Enable crash reporting via Crashpad" OFF)
option(ENABLE_CCACHE "Enable ccache wrapper" OFF)
option(ENABLE_APPIMAGE "Build in AppImage mode" OFF)
option(OSTARA_STATIC_BUILD "Build libs as static (matches QBS buildconfig.staticBuild)" ON)

# `enable_testing()` doit être appelé dans le dossier racine : sinon `ctest` ne trouve aucun essai, même
# quand les `add_test()` des sous-dossiers sont bien écrits. Il ne fait rien tant que les essais sont OFF.
if(BUILD_TESTING)
    enable_testing()
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# C++20 — obligatoire
# ═══════════════════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_SEPARATE_DEBUG_INFORMATION ON)

# ═══════════════════════════════════════════════════════════════════════════════
# Qt auto-magic
# ═══════════════════════════════════════════════════════════════════════════════
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ═══════════════════════════════════════════════════════════════════════════════
# Qt 6.11
# ═══════════════════════════════════════════════════════════════════════════════
# Aucun chemin n'est imposé : le projet doit se configurer sur n'importe quelle machine.
# Trois façons de désigner Qt, dans cet ordre :
#   1. -DCMAKE_PREFIX_PATH=/chemin/vers/Qt/6.11.1/<kit>   (prioritaire, écrase tout)
#   2. la variable d'environnement QT_DIR
#   3. les emplacements habituels de l'installeur Qt, pour les trois plateformes
if(NOT CMAKE_PREFIX_PATH AND DEFINED ENV{QT_DIR})
    set(CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
endif()
if(NOT CMAKE_PREFIX_PATH AND DEFINED ENV{HOME})
    list(APPEND CMAKE_PREFIX_PATH
        "$ENV{HOME}/Qt/6.11.1/macos"        # macOS, universel
        "$ENV{HOME}/Qt/6.11.1/gcc_64"       # Linux
        "$ENV{HOME}/Qt/6.11.1/msvc2022_64"  # Windows
    )
endif()
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    Concurrent
    PrintSupport
    OpenGLWidgets
    Network
    LinguistTools
)

# ═══════════════════════════════════════════════════════════════════════════════
# Dépendances externes
# ═══════════════════════════════════════════════════════════════════════════════
find_package(XercesC REQUIRED)
# OpenSSL n'est pas une dépendance du projet : aucun fichier source ne l'utilise
# (vérifié sur les symboles importés du binaire déployé — zéro). Les deux lignes de
# liaison ont été retirées le 27-10-2026 ; elles empêchaient toute compilation hors
# de macOS, où find_package n'était même pas appelé.

# ═══════════════════════════════════════════════════════════════════════════════
# Définitions globales (équivalent buildconfig.qbs)
# ═══════════════════════════════════════════════════════════════════════════════
add_compile_definitions(
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060900
    QT_MESSAGELOGCONTEXT
    QT_NO_FOREACH
    QT_USE_NODISCARD_FILE_OPEN
)

# Release : désactiver les assertions
add_compile_definitions($<$<NOT:$<CONFIG:Debug>>:V_NO_ASSERT>)

# Crashpad
if(ENABLE_CRASHPAD)
    add_compile_definitions(CRASH_REPORTING)
endif()

# AppImage (linux)
if(ENABLE_APPIMAGE)
    add_compile_definitions(APPIMAGE)
endif()

# Chemins d'installation Unix (BINDIR, DATADIR, PKGDATADIR)
# PKGDATADIR est nécessaire aussi sur macOS (vcommonsettings.cpp:242, vabstractapplication.cpp)
if(UNIX AND NOT APPLE)
    add_compile_definitions(
        BINDIR="${CMAKE_INSTALL_PREFIX}/bin"
        DATADIR="${CMAKE_INSTALL_PREFIX}/share"
        PKGDATADIR="${CMAKE_INSTALL_PREFIX}/share/ostara"
    )
elseif(APPLE)
    add_compile_definitions(
        PKGDATADIR="${CMAKE_INSTALL_PREFIX}/share/ostara"
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Compiler flags
# ═══════════════════════════════════════════════════════════════════════════════
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wfloat-equal
        -Wformat=2
        -Wcast-qual
        -Wcast-align
        -Woverloaded-virtual
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wno-covered-switch-default
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wfloat-equal
        -Wformat=2
        -Wcast-qual
        -Wcast-align
        -Woverloaded-virtual
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wno-covered-switch-default
    )
endif()

# Warnings as errors (comme le QBS)
if(MSVC)
    add_compile_options(/WX)
else()
    add_compile_options(-Werror)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Codegen — vcsRepoState.h (équivalent QBS vcs2)
# ═══════════════════════════════════════════════════════════════════════════════
execute_process(
    COMMAND git describe --always --long HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OSTARA_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(OSTARA_GIT_DESCRIBE)
    string(REGEX MATCH "^(.*)-([0-9]+)-(g[0-9a-f]+)$" _match "${OSTARA_GIT_DESCRIBE}")
    if(_match)
        set(OSTARA_GIT_STATE "${OSTARA_GIT_DESCRIBE}")
        set(OSTARA_GIT_TAG "${CMAKE_MATCH_1}")
        set(OSTARA_GIT_DISTANCE "${CMAKE_MATCH_2}")
        set(OSTARA_GIT_REVISION "${CMAKE_MATCH_3}")
    else()
        # No tags — fallback to raw SHA
        set(OSTARA_GIT_STATE "${OSTARA_GIT_DESCRIBE}")
        set(OSTARA_GIT_TAG "none")
        set(OSTARA_GIT_DISTANCE "none")
        set(OSTARA_GIT_REVISION "${OSTARA_GIT_DESCRIBE}")
    endif()
else()
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE OSTARA_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(OSTARA_GIT_STATE "${OSTARA_GIT_REVISION}")
    set(OSTARA_GIT_TAG "none")
    set(OSTARA_GIT_DISTANCE "none")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/vcsRepoState.h.in
    ${CMAKE_BINARY_DIR}/generated/vcsRepoState.h
    @ONLY
)

# Include directory for generated headers
include_directories(${CMAKE_BINARY_DIR}/generated)

# ═══════════════════════════════════════════════════════════════════════════════
# Sous-projets
# ═══════════════════════════════════════════════════════════════════════════════

# Le grec et le roumain n'ont pas de catalogue Qt : qt_add_translations avertit à chaque
# configuration. Le drapeau doit être posé ici, dans la portée racine — posé dans
# src/app/ostara/CMakeLists.txt, il n'est pas visible de l'appel différé qui émet l'alerte.
set(QT_NO_MISSING_CATALOG_LANGUAGE_WARNING ON)

add_subdirectory(src)
