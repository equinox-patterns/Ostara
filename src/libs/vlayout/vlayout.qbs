VLib {
    Depends { name: "Qt"; submodules: ["concurrent", "svg"] }
    Depends { name: "VMiscLib" }
    Depends { name: "VThemeLib" }
    Depends { name: "VGeometryLib" }
    Depends { name: "VPatternDBLib" }
    Depends { name: "VObjLib" }
    Depends { name: "QMUParserLib" }
    Depends { name: "VDXFLib" }
    Depends { name: "VHPGLLib"}
    Depends { name: "IFCLib" }
    Depends { name: "VWidgetsLib" }
    Depends { name: "VFormatLib" }

    name: "VLayoutLib"
    files: {
        var files = [
            "vlayoutexporter.h",
            "vlayoutdef.h",
            "vlayoutpoint.h",
            "vsapoint.h",
            "vgraphicsfillitem.h",
            "vabstractpiece.h",
            "vabstractpiece_p.h",
            "vlayoutpiece.h",
            "vlayoutpiece_p.h",
            "vlayoutpiecepath.h",
            "vlayoutpiecepath_p.h",
            "vrawsapoint.h",
            "vlayoutexporter.cpp",
            "vlayoutpoint.cpp",
            "vgraphicsfillitem.cpp",
            "vabstractpiece.cpp",
            "vlayoutpiece.cpp",
            "vlayoutpiecepath.cpp",
            "vrawsapoint.cpp",
            "vboundary.h",
            "vboundary.cpp",
            "vfoldline.h",
            "vfoldline.cpp"
        ];

        return files;
    }

    Group {
        name: "dialogs"
        prefix: "dialogs/"
        files: [
            "vabstractlayoutdialog.h",
            "watermarkwindow.h",
            "vabstractlayoutdialog.cpp",
            "watermarkwindow.cpp",
            "watermarkwindow.ui",
        ]
    }

    Group {
        name: "Resources"
        prefix: "share/"
        files: [
            "icons.qrc",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
