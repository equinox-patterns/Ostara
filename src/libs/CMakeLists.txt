# Libs — ordre feuille → racine dicté par les dépendances

# Couche 0 — Feuilles (zéro dépendance interne)
add_subdirectory(vexception)
add_subdirectory(vdispatch)
add_subdirectory(vtextstream)
add_subdirectory(vbackport)
add_subdirectory(graaflib)
add_subdirectory(qmuparser)

# Couche 1 — VMisc + VTheme (dépendent de VExceptionLib)
add_subdirectory(vmisc)
add_subdirectory(vtheme)

# Couche 2 — IFCLib + VCrashHandlerLib + géométrie/formats
add_subdirectory(ifc)
add_subdirectory(vcrashhandler)
add_subdirectory(vgeometry)
add_subdirectory(vdxf)
add_subdirectory(vobj)
add_subdirectory(vformat)
add_subdirectory(vhpgl)

# Couche 3 — Pattern DB + Property Explorer + Widgets
add_subdirectory(vpatterndb)
add_subdirectory(vpropertyexplorer)
add_subdirectory(vwidgets)

# Couche 4 — Layout + Tools + Fervor + Sanctions
add_subdirectory(vlayout)
add_subdirectory(vtools)
add_subdirectory(fervor)
add_subdirectory(vganalytics)

# Couche 5 — VTestLib (dépend des apps)
# add_subdirectory(vtest)
