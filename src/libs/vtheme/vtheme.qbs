import qbs.FileInfo

VLib {
    Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }
    Depends { name: "buildconfig" }
    Depends { name: "VMiscLib" }

    name: "VThemeLib"

    files: [
        "themeDef.h",
        "themeDef.cpp",
        "vtheme.h",
        "vtheme.cpp",
        "vapplicationstyle.h",
        "vapplicationstyle.cpp",
        "vscenestylesheet.h",
        "vscenestylesheet.cpp",
        "vstylesheetstyle.h",
        "vstylesheetstyle.cpp",
        "vtoolstyle.h",
        "vtoolstyle.cpp",
        "vpatternpiecestyle.h",
        "vpatternpiecestyle.cpp",
        "vmanuallayoutstyle.h",
        "vmanuallayoutstyle.cpp",
    ]

    Group {
        name: "Mac utils"
        files: [
            "macutils.h",
            "macutils.mm",
        ]
        condition: qbs.targetOS.contains("macos")
    }

    // Private: need vcommonsettings.h, vabstractapplication.h, def.h, compatibility.h from vmisc
    cpp.includePaths: [FileInfo.joinPaths(product.sourceDirectory, "..", "vmisc")]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }
        Depends { name: "VMiscLib" }
        cpp.includePaths: [
            exportingProduct.sourceDirectory,
            FileInfo.joinPaths(exportingProduct.sourceDirectory, "..", "vmisc"),
        ]
    }
}
