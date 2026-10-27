# Les essais du modèle — le banc historique, porté du qbs vers le CMake le 27-10-2026.
#
# Il vivait dans `src/test/ValentinaTest/`, en une seule cible qbs jamais construite par ce CMake : trente-
# quatre fichiers d'essai qui ne tournaient donc nulle part. Chaque `tst_*.cpp` a maintenant son exécutable,
# comme dans `OstaraTest` — la panne de l'un n'emporte pas les autres, et `ctest` les lance un par un.
#
# Le `main` est commun (`testrunner.cpp`) : il monte les schémas XSD, Xerces et l'application de test, puis
# instancie la classe choisie par deux définitions de compilation. Le nom de la classe n'étant pas
# dérivable du nom de fichier, la liste ci-dessous donne les deux.

find_package(Qt6 REQUIRED COMPONENTS Test)

# Les données d'essai (249 fichiers : polygones, cas de couture, patrons) sont dans `share/test_data.qrc`.
# Cinq essais les lisent par `:/…`. Une bibliothèque statique plutôt que trente-quatre copies : sans
# `InitOstaraTestData()`, l'éditeur de liens jette ses ressources avec l'objet qui les porte.
add_library(OstaraTestData STATIC testdata_init.cpp share/test_data.qrc)
set_target_properties(OstaraTestData PROPERTIES AUTORCC ON)
target_link_libraries(OstaraTestData PRIVATE Qt6::Core)

# `tst_vtextstream` compare ce que donne la lecture d'un flux selon qu'il vient d'un tuyau ou d'un
# fichier : il lui faut trois petits programmes qui lisent leur entrée standard. Ils existaient déjà
# (37 à 49 lignes), écrits pour qbs ; il ne manquait que des cibles CMake. Tous trois sont écrits dans
# le même dossier, parce que l'essai s'y place (`QDir::setCurrent`) avant de lancer ses sous-processus.
set(OSTARA_STDIN_HELPERS stdinProcess readLineStdinProcess readAllStdinProcess)

foreach(helper IN LISTS OSTARA_STDIN_HELPERS)
    add_executable(${helper} vtextstream/${helper}/main.cpp)
    set_target_properties(${helper} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtextstream)
    target_link_libraries(${helper} PRIVATE VTextStreamLib Qt6::Core)
endforeach()


# fichier:classe
set(OSTARA_MODEL_TESTS
    tst_dxf:TST_DXF
    tst_findpoint:TST_FindPoint
    tst_formulacache:TST_FormulaCache
    tst_misc:TST_Misc
    tst_nameregexp:TST_NameRegExp
    tst_qmutokenparser:TST_QmuTokenParser
    tst_readval:TST_ReadVal
    tst_renametoken:TST_RenameToken
    tst_vabstractcurve:TST_VAbstractCurve
    tst_vabstractpattern:TST_VAbstractPattern
    tst_vabstractpiece:TST_VAbstractPiece
    tst_varc:TST_VArc
    tst_vboundary:TST_VBoundary
    tst_vcommandline:TST_VCommandLine
    tst_vcubicbezierpath:TST_VCubicBezierPath
    tst_vdomdocument:TST_VDomDocument
    tst_vellipticalarc:TST_VEllipticalArc
    tst_vfoldline:TST_VFoldLine
    tst_vgobject:TST_VGObject
    tst_vlayoutdetail:TST_VLayoutDetail
    tst_vlockguard:TST_VLockGuard
    tst_vmaingraphicsscene:TST_VMainGraphicsScene
    tst_vmeasurements:TST_VMeasurements
    tst_vpatterngraph:TST_VPatternGraph
    tst_vpiece:TST_VPiece
    tst_vpointf:TST_VPointF
    tst_vspline:TST_VSpline
    tst_vsplinepath:TST_VSplinePath
    tst_vtextcodec:TST_VTextCodec
    tst_vtextstream:TST_VTextStream
    tst_vtheme:TST_VTheme
    tst_vtooluniondetails:TST_VToolUnionDetails
    tst_vtranslatevars:TST_VTranslateVars
    tst_xsdschema:TST_XSDShema
)

# Les essais qui lisent `share/test_data.qrc` par `:/…`
set(OSTARA_MODEL_TESTS_DATA
    tst_vabstractcurve
    tst_vabstractpiece
    tst_vboundary
    tst_vpiece
)

# Le banc touche au modèle, à la géométrie, au XML et au rendu : la liste est large parce que les essais
# le sont. Le lien est statique, l'inutile n'est pas embarqué.
set(OSTARA_MODEL_TEST_LIBS
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
    Qt6::Test
    Qt6::PrintSupport
    IFCLib
    QMUParserLib
    VDXFLib
    VExceptionLib
    VFormatLib
    VGeometryLib
    VLayoutLib
    VMiscLib
    VObjLib
    VPatternDBLib
    VTextStreamLib
    VTestLib
    VThemeLib
    VToolsLib
    VWidgetsLib
)

foreach(entry IN LISTS OSTARA_MODEL_TESTS)
    string(REPLACE ":" ";" parts "${entry}")
    list(GET parts 0 test)
    list(GET parts 1 klass)

    add_executable(${test} testrunner.cpp ${test}.cpp ${test}.h)

    target_include_directories(${test} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/libs
        ${CMAKE_SOURCE_DIR}/src/app/ostara
        ${CMAKE_SOURCE_DIR}/src/app/ostara/core)

    target_compile_definitions(${test} PRIVATE
        OSTARA_TEST_HEADER="${test}.h"
        OSTARA_TEST_CLASS=${klass})

    if(test IN_LIST OSTARA_MODEL_TESTS_DATA)
        target_compile_definitions(${test} PRIVATE OSTARA_TEST_DATA)
        target_link_libraries(${test} PRIVATE OstaraTestData)
    endif()

    # Les chemins des trois programmes à tuyau, donnés par leur cible : ils n'existent qu'après la
    # construction, et l'essai les lance en sous-processus.
    if(test STREQUAL "tst_vtextstream")
        target_compile_definitions(${test} PRIVATE
            STDIN_PROCESS_BUILDDIR="$<TARGET_FILE:stdinProcess>"
            READ_LINE_STDIN_PROCESS_BUILDDIR="$<TARGET_FILE:readLineStdinProcess>"
            READ_ALL_STDIN_PROCESS_BUILDDIR="$<TARGET_FILE:readAllStdinProcess>")
        add_dependencies(${test} ${OSTARA_STDIN_HELPERS})
    endif()

    target_link_libraries(${test} PRIVATE ${OSTARA_MODEL_TEST_LIBS})

    add_test(NAME ${test} COMMAND ${test})
endforeach()
