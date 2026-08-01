# VDXFLib — DXF import/export (includes vendored libdxfrw)

set(VDXF_SOURCES
    vdxfengine.cpp
    vdxfpaintdevice.cpp
    dxiface.cpp
    dxfdef.cpp
    vdxfengine.h
    vdxfpaintdevice.h
    dxfdef.h
    dxiface.h
)

# Vendored libdxfrw
set(VDXF_LIBDXFRW
    libdxfrw/drw_base.cpp
    libdxfrw/intern/drw_dbg.cpp
    libdxfrw/intern/drw_textcodec.cpp
    libdxfrw/intern/dxfreader.cpp
    libdxfrw/intern/dxfwriter.cpp
    libdxfrw/drw_classes.cpp
    libdxfrw/drw_entities.cpp
    libdxfrw/drw_header.cpp
    libdxfrw/drw_objects.cpp
    libdxfrw/libdxfrw.cpp
    libdxfrw/drw_reserve.h
    libdxfrw/intern/make_unique.h
    libdxfrw/intern/drw_dbg.h
    libdxfrw/intern/drw_textcodec.h
    libdxfrw/intern/dxfreader.h
    libdxfrw/intern/dxfwriter.h
    libdxfrw/drw_base.h
    libdxfrw/drw_classes.h
    libdxfrw/drw_entities.h
    libdxfrw/drw_header.h
    libdxfrw/drw_interface.h
    libdxfrw/drw_objects.h
    libdxfrw/libdxfrw.h
    libdxfrw/main_doc.h
)

add_library(VDXFLib STATIC ${VDXF_SOURCES} ${VDXF_LIBDXFRW})

target_include_directories(VDXFLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VDXFLib
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Xml
        VMiscLib
        VThemeLib
)

target_precompile_headers(VDXFLib PRIVATE stable.h)

# Suppress warnings in vendored code
set_source_files_properties(
    ${VDXF_LIBDXFRW}
    PROPERTIES COMPILE_FLAGS "-Wno-sign-conversion -Wno-conversion -Wno-old-style-cast -Wno-float-equal -Wno-cast-qual"
)
