# Ostara — Main pattern editor app (ex-"Valentina")

file(GLOB OSTARA_SOURCES *.cpp *.h *.ui)
file(GLOB OSTARA_CORE core/*.cpp core/*.h)
file(GLOB_RECURSE OSTARA_DIALOGS dialogs/*.cpp dialogs/*.h dialogs/*.ui)
file(GLOB_RECURSE OSTARA_XML xml/*.cpp xml/*.h)
file(GLOB_RECURSE OSTARA_WIDGETS widgets/*.cpp widgets/*.h)

# QRC resources
set(OSTARA_QRC
    share/resources/toolcursor.qrc
    share/resources/toolicon.qrc
    share/resources/fonts.qrc
    share/resources/drawtool_icons.qrc
    share/resources/export_icons.qrc
)

# ══════════════════════════════════════════════════════════════════════════
# Trame — le socle de nesting (bibliothèque STATIQUE), construit dans ce build
# ══════════════════════════════════════════════════════════════════════════
# add_subdirectory HORS ARBRE : source dir en ABSOLU, binary dir EXPLICITE —
# même montage que Rabnest/CMakeLists.txt:18-21. EXCLUDE_FROM_ALL : les trois
# outils de Trame (svg2ocgpdf, svg2pdf, nestbench) n'ont pas à être construits
# ici ; seule la bibliothèque l'est, parce qu'Ostara la lie.
# Trame a sa propre politique de compilation. Ce qui est posé à la racine
# d'Ostara se propage à tout sous-dossier ajouté ensuite — donc à Trame et à
# Clipper2, qui ne sont pas écrits pour ça. Deux fuites constatées :
#   • options (CMakeLists.txt:102-140, -Werror en tête) — Trame n'est pas
#     écrit pour -Werror -Wconversion ;
#   • définitions (CMakeLists.txt:58-67) — QT_DISABLE_DEPRECATED_BEFORE y
#     supprime la surcharge de QDomDocument::setContent utilisée par
#     SvgToOcgPdf.cpp.
# Trame étant un socle partagé, il ne doit pas hériter de la politique d'un
# consommateur. On vide donc les deux propriétés de répertoire le temps de
# l'ajouter — le sous-dossier en hérite une copie à sa création — puis on les
# restaure pour les cibles d'Ostara, créées juste après.
get_property(_ostaraDirOptions DIRECTORY PROPERTY COMPILE_OPTIONS)
get_property(_ostaraDirDefinitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)

set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS "")

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Trame"
    "${CMAKE_BINARY_DIR}/Trame"
    EXCLUDE_FROM_ALL
)

set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "${_ostaraDirOptions}")
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS "${_ostaraDirDefinitions}")

# Clipper2 est un tiers, atteint depuis Ostara par les en-têtes publics de Trame
# (model/Piece.h inclut <clipper2/clipper.h>). Ses en-têtes ne survivent pas à la
# politique de warning d'Ostara (-Wfloat-equal en tête). Déclarés SYSTEM, ils sont
# compilés sans ces avertissements, quoi que demande le consommateur.
# Le répertoire doit rester déclaré dans INTERFACE_INCLUDE_DIRECTORIES pour être
# cherché ; INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ne fait que le marquer système.
if(TARGET Clipper2)
    get_target_property(_clipperIncludes Clipper2 INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(Clipper2 PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_clipperIncludes}"
    )
endif()

qt_add_executable(Ostara
    ${OSTARA_SOURCES}
    ${OSTARA_CORE}
    ${OSTARA_DIALOGS}
    ${OSTARA_XML}
    ${OSTARA_WIDGETS}
    ${OSTARA_QRC}
)

target_include_directories(Ostara PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Ostara PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    Qt6::Concurrent
    VPatternDBLib
    VWidgetsLib
    FervorLib
    IFCLib
    VLayoutLib
    VToolsLib
    VFormatLib
    VDispatchLib
    VMiscLib
    VThemeLib
    VGAnalyticsLib
    XercesC::XercesC
    OpenSSL::SSL
    OpenSSL::Crypto
    Trame
)

# macOS bundle
if(APPLE)
    set_target_properties(Ostara PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ostara.ostara"
        MACOSX_BUNDLE_BUNDLE_NAME "Ostara"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    )
endif()

# Define VALENTINA_PATH and TRANSLATIONS_DIR for VTestLib consumers
target_compile_definitions(Ostara PUBLIC
    VALENTINA_PATH="$<TARGET_FILE:Ostara>"
    TRANSLATIONS_DIR="${CMAKE_SOURCE_DIR}/share/translations"
)

target_precompile_headers(Ostara PRIVATE stable.h)

# Help AUTOUIC find ui_ headers
target_include_directories(Ostara PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/configpages
)

# ═══════════════════════════════════════════════════════════
# Translations — embarquées dans le binaire (ressources Qt)
# MERGE_QT_TRANSLATIONS fusionne les traductions Qt (qtbase, etc.)
# dans les .qm de l'app. Ceci configure aussi automatiquement
# CFBundleLocalizations dans Info.plist sur macOS.
# ═══════════════════════════════════════════════════════════

set(OSTARA_TS_DIR "${CMAKE_SOURCE_DIR}/share/translations")

set(OSTARA_LOCALES
    cs_CZ de_DE el_GR es_ES fi_FI fr_FR hr_HR hu_HU
    id_ID it_IT ja_JP nl_NL pl_PL pt_BR ro_RO uk_UA zh_CN
)

set(OSTARA_TS_FILES)
foreach(locale ${OSTARA_LOCALES})
    list(APPEND OSTARA_TS_FILES ${OSTARA_TS_DIR}/ostara_${locale}.ts)
endforeach()

# Associer chaque .ts à sa locale pour MERGE_QT_TRANSLATIONS
foreach(locale ${OSTARA_LOCALES})
    set_source_files_properties(
        ${OSTARA_TS_DIR}/ostara_${locale}.ts
        PROPERTIES QT_I18N_TRANSLATED_LANGUAGES ${locale}
    )
endforeach()

# Supprimer les warnings pour les locales sans traduction Qt (id_ID, ro_RO, el_GR)
set(QT_NO_MISSING_CATALOG_LANGUAGE_WARNING ON)

qt_add_translations(Ostara
    TS_FILES ${OSTARA_TS_FILES}
    MERGE_QT_TRANSLATIONS
)

# ═══════════════════════════════════════════════════════════
# macOS bundle resources
# ═══════════════════════════════════════════════════════════
if(APPLE)
    # SVG fonts
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/app/share/svgfonts/"
        DESTINATION "Ostara.app/Contents/Resources/svgfonts"
    )

    # L'icône de l'application. Elle a été longtemps absente : CFBundleIconFile partait vide et le
    # paquet n'avait aucun .icns, donc le Finder affichait l'icône générique et LaunchServices
    # n'avait rien à dessiner. Le .icns est engendré depuis share/icons/app/Ostara.svg.
    set_target_properties(Ostara PROPERTIES MACOSX_BUNDLE_ICON_FILE "Ostara.icns")
    install(FILES "${CMAKE_SOURCE_DIR}/share/icons/app/Ostara.icns"
        DESTINATION "Ostara.app/Contents/Resources"
    )

    # qt.conf
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPlugins = PlugIns\n"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        DESTINATION "Ostara.app/Contents/Resources"
    )

    # Install internal dylibs into Frameworks (before macdeployqt)
    install(FILES
        $<TARGET_FILE:VPropertyExplorerLib>
        $<TARGET_FILE:QMUParserLib>
        DESTINATION "Ostara.app/Contents/Frameworks"
    )

    # Install the bundle
    install(TARGETS Ostara BUNDLE DESTINATION .)

    # macdeployqt — bundles Qt + Xerces + OpenSSL
    qt_generate_deploy_app_script(
        TARGET Ostara
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
else()
    install(TARGETS Ostara DESTINATION bin)
endif()
