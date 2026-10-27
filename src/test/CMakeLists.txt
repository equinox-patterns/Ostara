# Les essais unitaires du produit.
#
# Deux bancs, tous deux construits **seulement** par `-DBUILD_TESTING=ON`, et dans un dossier de
# construction à part — jamais celui de l'application, qui reste sans essais :
#
#   OstaraTest       — les essais écrits pour le CMake.
#   OstaraModelTest  — le banc historique du modèle, porté du qbs au CMake le 27-10-2026 : les
#                      trente-quatre `tst_*.cpp` qui ne tournaient nulle part jusque-là, tous les
#                      trente-quatre lancés par `ctest`.
add_subdirectory(OstaraTest)
add_subdirectory(OstaraModelTest)
