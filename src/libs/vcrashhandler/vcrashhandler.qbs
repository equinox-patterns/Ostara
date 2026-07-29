import qbs.FileInfo

VLib {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "buildconfig" }
    Depends { name: "VMiscLib" }

    Depends {
        name: "crashpad"
        condition: buildconfig.useConanPackages && buildconfig.conanCrashReportingEnabled
    }

    Properties {
        condition: buildconfig.useConanPackages && buildconfig.conanCrashReportingEnabled && qbs.targetOS.contains("windows") && qbs.toolchain.contains("msvc")
        cpp.dynamicLibraries: ["Advapi32"]
    }

    Properties {
        condition: buildconfig.useConanPackages && buildconfig.conanCrashReportingEnabled && qbs.targetOS.contains("macos")
        cpp.libraryPaths: ["/usr/lib"]
        cpp.dynamicLibraries: ["bsm"]
        cpp.frameworks: ["AppKit", "Security"]
    }

    Properties {
        condition: buildconfig.useConanPackages && buildconfig.conanCrashReportingEnabled && qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        cpp.dynamicLibraries: ["dl"]
    }

    name: "VCrashHandlerLib"

    files: [
        "crashhandler.h",
        "crashhandler.cpp",
        "vcrashpaths.h",
        "vcrashpaths.cpp",
    ]

    // Private: need projectversion.h, vcommonsettings.h, compatibility.h, appimage.h from vmisc
    cpp.includePaths: [FileInfo.joinPaths(product.sourceDirectory, "..", "vmisc")]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core"] }
        Depends { name: "VMiscLib" }
        Depends {
            name: "crashpad"
            condition: buildconfig.useConanPackages && buildconfig.conanCrashReportingEnabled
        }
        cpp.includePaths: [
            exportingProduct.sourceDirectory,
            FileInfo.joinPaths(exportingProduct.sourceDirectory, "..", "vmisc"),
        ]
    }
}
