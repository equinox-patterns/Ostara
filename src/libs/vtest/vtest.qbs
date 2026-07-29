VLib {
    Depends { name: "Qt"; submodules: ["testlib"] }
    Depends { name: "VMiscLib" }
    Depends { name: "VThemeLib" }
    Depends { name: "VGeometryLib" }
    Depends { name: "VPatternDBLib" }
    Depends { name: "VLayoutLib" }
    Depends { name: "Tape" }
    Depends { name: "Valentina" }
    Depends { name: "Puzzle" }

    name: "VTestLib"
    files: [
        "abstracttest.cpp",
        "abstracttest.h",
    ]

    Export {
        Depends { name: "cpp" }
        Depends { name: "VMiscLib" }
    Depends { name: "VThemeLib" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
