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
    share/resources/licenses.qrc
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

# ─── Où est Trame ? ──────────────────────────────────────────────────────────
# Trame est un dépôt séparé. Trois cas, dans l'ordre :
#   1. un dossier voisin, ce qui est l'usage courant en développement ;
#   2. -DOSTARA_TRAME_REPOSITORY=<url> [-DOSTARA_TRAME_TAG=<branche ou étiquette>]
#      pour une machine neuve ou une CI, qui le télécharge ;
#   3. sinon on s'arrête net, avec un message qui dit quoi faire — plutôt qu'une
#      erreur de compilation incompréhensible trois cents lignes plus loin.
set(_ostaraTrameDir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Trame")
if(NOT EXISTS "${_ostaraTrameDir}/CMakeLists.txt")
    if(NOT OSTARA_TRAME_REPOSITORY)
        message(FATAL_ERROR
            "Trame est introuvable : ni le dossier voisin "
            "'${_ostaraTrameDir}', ni -DOSTARA_TRAME_REPOSITORY. "
            "Clonez Trame à côté d'Ostara, ou passez "
            "-DOSTARA_TRAME_REPOSITORY=<url du dépôt Trame>.")
    endif()
    if(NOT OSTARA_TRAME_TAG)
        set(OSTARA_TRAME_TAG "")
    endif()
    include(FetchContent)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)   # FetchContent_Populate reste utilisé ici
    endif()
    # Sans GIT_TAG, git prend la branche par défaut du dépôt : rien à deviner, et
    # ça marche que Trame soit sur « main » ou sur « master ».
    set(_ostaraTrameFetchArgs GIT_REPOSITORY "${OSTARA_TRAME_REPOSITORY}")
    if(OSTARA_TRAME_TAG)
        list(APPEND _ostaraTrameFetchArgs GIT_TAG "${OSTARA_TRAME_TAG}")
    endif()
    FetchContent_Declare(Trame ${_ostaraTrameFetchArgs})
    FetchContent_GetProperties(Trame)
    if(NOT Trame_POPULATED)
        FetchContent_Populate(Trame)
    endif()
    set(_ostaraTrameDir "${trame_SOURCE_DIR}")
endif()

add_subdirectory(
    "${_ostaraTrameDir}"
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
    IFCLib
    VLayoutLib
    VToolsLib
    VFormatLib
    VDispatchLib
    VMiscLib
    VThemeLib
    VGAnalyticsLib
    XercesC::XercesC
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
    it_IT nl_NL pl_PL pt_BR ro_RO uk_UA zh_CN
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

qt_add_translations(Ostara
    TS_FILES ${OSTARA_TS_FILES}
    MERGE_QT_TRANSLATIONS
)

# ═══════════════════════════════════════════════════════════
# macOS bundle resources
# ═══════════════════════════════════════════════════════════
if(APPLE)
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

    # Les textes de licence, en vrais fichiers à côté de leur copie embarquée. La GPL exige qu'une
    # copie de la licence accompagne le programme ; la ressource Qt les rend lisibles dans
    # l'application, ce dossier les rend lisibles sans l'ouvrir — un `grep` sur le paquet suffit.
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/share/licenses/"
        DESTINATION "Ostara.app/Contents/Resources/licenses"
        FILES_MATCHING PATTERN "*.txt"
    )

    # Ce que la GPL et les licences tierces demandent de joindre, en clair dans le paquet : la
    # notice (marque, origine, offre de source), l'inventaire des composants et le copyright.
    install(FILES "${CMAKE_SOURCE_DIR}/NOTICE"
                  "${CMAKE_SOURCE_DIR}/COPYRIGHT.md"
                  "${CMAKE_SOURCE_DIR}/AUTHORS.txt"
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

    # macdeployqt — bundles Qt + Xerces
    qt_generate_deploy_app_script(
        TARGET Ostara
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
else()
    # Les bibliothèques internes sont des .so séparés : sans elles dans le paquet, le binaire
    # ne démarre pas et linuxdeploy refuse de construire l'AppImage.
    set_target_properties(Ostara PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    install(TARGETS Ostara DESTINATION bin)
    install(TARGETS VPropertyExplorerLib QMUParserLib LIBRARY DESTINATION lib)
    install(FILES "${CMAKE_SOURCE_DIR}/NOTICE"
                  "${CMAKE_SOURCE_DIR}/COPYRIGHT.md"
                  "${CMAKE_SOURCE_DIR}/AUTHORS.txt"
                  "${CMAKE_SOURCE_DIR}/LICENSE"
        DESTINATION "share/doc/ostara"
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/share/licenses/"
        DESTINATION "share/doc/ostara/licenses"
        FILES_MATCHING PATTERN "*.txt"
    )

    # ── Types de fichiers ─────────────────────────────────────────────────────
    # Trois pièces, et il faut les trois : le XML MIME déclare le type, l'icône du
    # thème le dessine, le .desktop dit que l'application sait l'ouvrir. Sans elles,
    # un `.ostara` reste un fichier inconnu dans le gestionnaire de fichiers — le
    # pendant exact du registre Windows, et macOS le fait déjà dans son Info.plist.
    #
    # `Exec=Ostara %F` (dans le .desktop) est ce qui passe le fichier à l'application
    # quand on double-clique dessus.
    install(FILES "${CMAKE_SOURCE_DIR}/share/ostara.desktop" DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/share/icons/app/Ostara.svg"
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME Ostara.svg
    )

    foreach(mimeType IN ITEMS pattern measurements cartouche)
        install(FILES "${CMAKE_SOURCE_DIR}/share/mime/application-x-ostara-${mimeType}.xml"
            DESTINATION share/mime/packages
        )
        install(FILES
                "${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/mimetypes/application-x-ostara-${mimeType}.svg"
            DESTINATION share/icons/hicolor/scalable/mimetypes
        )
        install(FILES
                "${CMAKE_SOURCE_DIR}/share/icons/hicolor/64x64/mimetypes/application-x-ostara-${mimeType}.png"
            DESTINATION share/icons/hicolor/64x64/mimetypes
        )
    endforeach()
endif()

# ═══════════════════════════════════════════════════════════
# Windows
# ═══════════════════════════════════════════════════════════
if(WIN32)
    # Sous-système GUI. Sans lui, Windows ouvre une fenêtre noire devant l'application au
    # double-clic, et c'est elle qui porte le processus : la fermer ferme Ostara. Rabnest
    # le pose depuis le début (Rabnest/CMakeLists.txt) ; Ostara jamais, et la 1.1.0 livrée
    # montrait cette console.
    # Le mode console reste utilisable : WinAttachConsole() (main.cpp) rattache le processus
    # au terminal quand on lance l'exe depuis un shell, donc --export et --version répondent.
    set_target_properties(Ostara PROPERTIES WIN32_EXECUTABLE TRUE)

    # L'icône est compilée dans l'exécutable, comme pour Rabnest : les raccourcis du bureau et
    # du menu Démarrer, ainsi que la fiche « Applications et fonctionnalités », lisent l'icône
    # de l'exécutable. Sans ressource, l'Explorateur affiche l'icône générique du programme.
    target_sources(Ostara PRIVATE "${CMAKE_SOURCE_DIR}/share/icons/app/ostara.rc")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/share/icons/app/ostara.rc" PROPERTIES
        COMPILE_FLAGS "-I\"${CMAKE_SOURCE_DIR}/share/icons/app\"")
endif()
