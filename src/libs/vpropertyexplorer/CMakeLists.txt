# VPropertyExplorerLib — Qt property browser/editor framework
# Always shared/dynamic (framework on macOS when not static build)
# Matches QBS VDynamicLib behavior

set(VPROPERTY_SOURCES
    qtcolorpicker.cpp
    qtcolorpicker.h
    vproperty.cpp
    vpropertydef.cpp
    vpropertydef.h
    vpropertydelegate.cpp
    vpropertyfactorymanager.cpp
    vpropertyformview.cpp
    vpropertyformwidget.cpp
    vpropertymodel.cpp
    vpropertyset.cpp
    vpropertytreeview.cpp
    vserializedproperty.cpp
    vstandardpropertyfactory.cpp
    checkablemessagebox.cpp
    vpropertyexplorer_global.h
    vpropertyfactorymanager_p.h
    vpropertytreeview_p.h
    vpropertyset_p.h
    vabstractpropertyfactory.h
    vfileproperty_p.h
    vwidgetproperty_p.h
    vpropertymodel_p.h
    vstandardpropertyfactory.h
    vpropertyformview_p.h
    vpropertytreeview.h
    vpropertyformwidget_p.h
    vpropertydelegate.h
    vproperty_p.h
    vpropertyformwidget.h
    vpropertyformview.h
    vpropertyset.h
    vpropertymodel.h
    vproperty.h
    vpropertyfactorymanager.h
    vserializedproperty.h
    vproperties.h
    checkablemessagebox.h
)

set(VPROPERTY_PLUGINS
    plugins/vtextproperty.cpp
    plugins/vwidgetproperty.cpp
    plugins/vemptyproperty.cpp
    plugins/vboolproperty.cpp
    plugins/vshortcutproperty.cpp
    plugins/vcolorproperty.cpp
    plugins/vshortcutpropertyeditor.cpp
    plugins/venumproperty.cpp
    plugins/vfileproperty.cpp
    plugins/vcolorpropertyeditor.cpp
    plugins/vfilepropertyeditor.cpp
    plugins/vnumberproperty.cpp
    plugins/Vector3d/vvector3dproperty.cpp
    plugins/vstringproperty.cpp
    plugins/vpointfproperty.cpp
    plugins/vobjectproperty.cpp
    plugins/vlinetypeproperty.cpp
    plugins/vlinecolorproperty.cpp
    plugins/vtextproperty.h
    plugins/vwidgetproperty.h
    plugins/vcolorproperty.h
    plugins/vboolproperty.h
    plugins/vcolorpropertyeditor.h
    plugins/vshortcutpropertyeditor.h
    plugins/vemptyproperty.h
    plugins/vshortcutproperty.h
    plugins/venumproperty.h
    plugins/vfilepropertyeditor.h
    plugins/vfileproperty.h
    plugins/vnumberproperty.h
    plugins/Vector3d/vvector3dproperty.h
    plugins/vstringproperty.h
    plugins/vpointfproperty.h
    plugins/vobjectproperty.h
    plugins/vlinetypeproperty.h
    plugins/vlinecolorproperty.h
    plugins/vlabelproperty.cpp
    plugins/vlabelproperty.h
)

if(APPLE AND NOT OSTARA_STATIC_BUILD)
    add_library(VPropertyExplorerLib SHARED ${VPROPERTY_SOURCES} ${VPROPERTY_PLUGINS})
    set_target_properties(VPropertyExplorerLib PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER "com.ostara.VPropertyExplorerLib"
        INSTALL_NAME_DIR "@rpath"
        VERSION "1.0.0"
        SOVERSION "1.0.0"
    )
else()
    add_library(VPropertyExplorerLib SHARED ${VPROPERTY_SOURCES} ${VPROPERTY_PLUGINS})
    set_target_properties(VPropertyExplorerLib PROPERTIES
        VERSION "1.0.0"
        SOVERSION "1.0.0"
    )
endif()

target_include_directories(VPropertyExplorerLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # src/libs base
)

target_link_libraries(VPropertyExplorerLib
    PUBLIC
        Qt6::Gui
        Qt6::Widgets
        VMiscLib
        VThemeLib
        IFCLib
)

target_compile_definitions(VPropertyExplorerLib PRIVATE VPROPERTYEXPLORER_LIBRARY)
target_precompile_headers(VPropertyExplorerLib PRIVATE stable.h)
