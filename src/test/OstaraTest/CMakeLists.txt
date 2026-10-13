# Qt Test n'est pas demandé par la construction de l'application : on le demande ici, pour ces essais-là seulement.
find_package(Qt6 REQUIRED COMPONENTS Test)

# Un exécutable par fichier `tst_*.cpp` : la panne de l'un n'emporte pas les autres.
set(OSTARA_TESTS tst_incrementtables tst_pieceenvelope)

# Les sources de l'application qu'un essai emporte avec lui. `OPieceDocument` n'est pas dans une bibliothèque — il
# est compilé avec l'exécutable — et l'enveloppe d'une pièce ne se vérifie pas sans lui.
set(tst_pieceenvelope_EXTRA
    ${CMAKE_SOURCE_DIR}/src/app/ostara/core/opiecedocument.cpp)

foreach(test IN LISTS OSTARA_TESTS)
    add_executable(${test} ${test}.cpp ${test}.h ${${test}_EXTRA})

    target_include_directories(${test} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs
        ${CMAKE_SOURCE_DIR}/src/app/ostara/core)

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

    add_test(NAME ${test} COMMAND ${test})
endforeach()
