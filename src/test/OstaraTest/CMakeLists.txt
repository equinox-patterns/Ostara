# Qt Test n'est pas demandé par la construction de l'application : on le demande ici, pour ces essais-là seulement.
find_package(Qt6 REQUIRED COMPONENTS Test)

# Un exécutable par fichier `tst_*.cpp` : la panne de l'un n'emporte pas les autres.
set(OSTARA_TESTS tst_incrementtables tst_pieceenvelope tst_vbackgroundpatternimage tst_vpatternconverter tst_expressionwalkers tst_oomtconverter tst_linerolenames tst_foldermove tst_folderpathsignal tst_dialogfit tst_dialoginventory)

# Les sources de l'application qu'un essai emporte avec lui. `OPieceDocument` n'est pas dans une bibliothèque — il
# est compilé avec l'exécutable — et l'enveloppe d'une pièce ne se vérifie pas sans lui.
set(tst_pieceenvelope_EXTRA
    ${CMAKE_SOURCE_DIR}/src/app/ostara/core/opiecedocument.cpp)

# Le déplacement de dossier est une opération de fichiers de la bibliothèque, compilée avec l'application.
set(tst_foldermove_EXTRA
    ${CMAKE_SOURCE_DIR}/src/app/ostara/core/olibrarystorage.cpp)

# L'inventaire mesuré construit **toutes** les dialogues du dépôt, et la liste est générée depuis les
# `.ui` à la configuration : une dialogue ajoutée demain entre dans l'inventaire sans que personne y pense.
file(GLOB_RECURSE OSTARA_UI_FILES RELATIVE ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/src/*.ui")

set(OSTARA_INVENTORY_UIS)
set(OSTARA_INVENTORY_INCLUDES)
set(OSTARA_INVENTORY_CASES)
set(OSTARA_INVENTORY_DIRS)

foreach(ui IN LISTS OSTARA_UI_FILES)
    file(READ "${CMAKE_SOURCE_DIR}/${ui}" ui_content)
    # Les pages, les docks et la fenêtre principale ne sont pas des dialogues : elles n'ont pas de cadre
    # à cadrer. Seul le `.ui` dont la racine est un `QDialog` entre dans l'inventaire.
    if(NOT ui_content MATCHES "<widget class=\"QDialog\"")
        continue()
    endif()

    string(REGEX MATCH "<class>([A-Za-z0-9_]+)</class>" _class_match "${ui_content}")
    if(NOT CMAKE_MATCH_1)
        continue()
    endif()
    set(class_name ${CMAKE_MATCH_1})

    get_filename_component(ui_name "${ui}" NAME_WE)
    get_filename_component(ui_dir "${CMAKE_SOURCE_DIR}/${ui}" DIRECTORY)

    list(APPEND OSTARA_INVENTORY_UIS "${CMAKE_SOURCE_DIR}/${ui}")
    list(APPEND OSTARA_INVENTORY_DIRS "${ui_dir}")
    string(APPEND OSTARA_INVENTORY_INCLUDES "#include \"ui_${ui_name}.h\"\n")
    string(APPEND OSTARA_INVENTORY_CASES
        "    {\"${class_name}\", [](QDialog &dialog) { Ui::${class_name} ui; ui.setupUi(&dialog); }},\n")
endforeach()

list(REMOVE_DUPLICATES OSTARA_INVENTORY_DIRS)

set(OSTARA_INVENTORY_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/dialoginventory_cases.cpp")
file(WRITE "${OSTARA_INVENTORY_SOURCE}.in"
"// Généré à la configuration depuis les .ui du dépôt — voir src/test/OstaraTest/CMakeLists.txt.
#include \"dialoginventory_cases.h\"

${OSTARA_INVENTORY_INCLUDES}
auto AllDialogCases() -> QVector<DialogCase>
{
    return {
${OSTARA_INVENTORY_CASES}    };
}
")
configure_file("${OSTARA_INVENTORY_SOURCE}.in" "${OSTARA_INVENTORY_SOURCE}" COPYONLY)

set(tst_dialoginventory_EXTRA ${OSTARA_INVENTORY_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/dialoginventory_codebuilt.cpp)
set(tst_dialoginventory_UIS ${OSTARA_INVENTORY_UIS})
set(tst_dialoginventory_INCLUDES ${OSTARA_INVENTORY_DIRS} ${CMAKE_SOURCE_DIR}/src/libs/vtools
    ${CMAKE_SOURCE_DIR}/src/libs)

message(STATUS "Inventaire des dialogues : ${OSTARA_INVENTORY_UIS}")

foreach(test IN LISTS OSTARA_TESTS)
    add_executable(${test} ${test}.cpp ${test}.h ${${test}_EXTRA} ${${test}_UIS})

    if(${test}_UIS)
        set_target_properties(${test} PROPERTIES AUTOUIC ON
            AUTOUIC_SEARCH_PATHS "${${test}_INCLUDES}")
    endif()

    target_include_directories(${test} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/libs
        ${CMAKE_SOURCE_DIR}/src/app/ostara/core
        ${${test}_INCLUDES})

    # Fixtures partagées (patrons et mesures d'essai), lues depuis la source et non depuis le build.
    target_compile_definitions(${test} PRIVATE
        OSTARA_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

    target_link_libraries(${test} PRIVATE
        Qt6::Core
        Qt6::Test
        IFCLib
        VFormatLib
        VGeometryLib
        VLayoutLib
        VMiscLib
        VPatternDBLib
        VTextStreamLib
        VWidgetsLib)

    if(TARGET OstaraApp)
        target_link_libraries(${test} PRIVATE OstaraApp)
    endif()

    add_test(NAME ${test} COMMAND ${test})
endforeach()
