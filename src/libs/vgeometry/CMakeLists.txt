# VGeometryLib — Geometry primitives (arcs, splines, beziers, points)

set(VGEOMETRY_SOURCES
    vgeometrydef.cpp
    vgobject.cpp
    vabstractcurve.cpp
    varc.cpp
    vlayoutplacelabel.cpp
    vpointf.cpp
    vspline.cpp
    vsplinepath.cpp
    vsplinepoint.cpp
    vellipticalarc.cpp
    vcubicbezier.cpp
    vabstractcubicbezier.cpp
    vabstractcubicbezierpath.cpp
    vcubicbezierpath.cpp
    vabstractarc.cpp
    vabstractbezier.cpp
    vplacelabelitem.cpp
    # Headers
    vgobject.h
    vgobject_p.h
    vabstractcurve.h
    varc.h
    varc_p.h
    vlayoutplacelabel.h
    vpointf.h
    vpointf_p.h
    vspline.h
    vspline_p.h
    vsplinepath.h
    vsplinepath_p.h
    vsplinepoint.h
    vsplinepoint_p.h
    vgeometrydef.h
    vellipticalarc.h
    vellipticalarc_p.h
    vabstractcurve_p.h
    vcubicbezier.h
    vcubicbezier_p.h
    vabstractcubicbezier.h
    vabstractcubicbezierpath.h
    vcubicbezierpath.h
    vcubicbezierpath_p.h
    vabstractarc.h
    vabstractarc_p.h
    vabstractbezier.h
    vplacelabelitem.h
    vplacelabelitem_p.h
    stable.h
)

add_library(VGeometryLib STATIC ${VGEOMETRY_SOURCES})

target_include_directories(VGeometryLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VGeometryLib
    PUBLIC
        Qt6::Gui
        Qt6::Concurrent
        VMiscLib
        IFCLib
        QMUParserLib
)

target_precompile_headers(VGeometryLib PRIVATE stable.h)
