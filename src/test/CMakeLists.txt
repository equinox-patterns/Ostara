# Les essais unitaires du produit.
#
# Le banc historique — `ValentinaTest`, une quarantaine de fichiers — est encore une **cible qbs**
# (`ValentinaTest/ValentinaTest.qbs`) : il n'est pas reconstruit ici. Ce dossier ne porte que les essais écrits
# pour le CMake, qui ne se construisent **que** par `-DBUILD_TESTING=ON`, dans un dossier de construction à part —
# jamais celui de l'application, qui reste sans essais.
add_subdirectory(OstaraTest)
