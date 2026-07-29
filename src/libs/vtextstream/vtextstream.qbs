VLib {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "buildconfig" }

    name: "VTextStreamLib"

    files: [
        "vtextstream.h",
        "vtextstream.cpp",
        "vtextstream_p.h",
    ]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core"] }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
