import qbs.Utilities

VLib {
    Depends { name: "Qt"; submodules: ["gui"] }
    Depends { name: "buildconfig" }

    name: "VBackportLib"

    files: {
        var files = []
        if (Utilities.versionCompare(Qt.core.version, "6.9") < 0) {
            files = ["qpainterstateguard.h", "qpainterstateguard.cpp"]
        }
        return files
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["gui", "core"] }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
