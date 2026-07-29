import qbs.FileInfo

VLib {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "buildconfig" }

    name: "VExceptionLib"

    files: [
        "vexception.h",
        "vexception.cpp",
    ]

    // Private: need defglobal.h from vmisc for V_NOEXCEPT_EXPR macro
    cpp.includePaths: [FileInfo.joinPaths(product.sourceDirectory, "..", "vmisc")]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core"] }
        cpp.includePaths: [
            exportingProduct.sourceDirectory,
            FileInfo.joinPaths(exportingProduct.sourceDirectory, "..", "vmisc"), // defglobal.h
        ]
    }
}
