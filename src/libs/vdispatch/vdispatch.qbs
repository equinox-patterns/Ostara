VLib {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "buildconfig" }

    name: "VDispatchLib"

    files: ["qt_dispatch.h"]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core"] }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
