import qbs.Utilities

VLib {
    Depends { name: "Qt"; submodules: ["core", "printsupport", "gui", "widgets"] }
    Depends { name: "buildconfig" }
    Depends { name: "VExceptionLib" }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.staticLibraries: ["User32"]
    }

    name: "VMiscLib"
    files: [
        "def.cpp",
        "testpath.cpp",
        "vabstractvalapplication.cpp",
        "vabstractapplication.cpp",
        "projectversion.cpp",
        "vcommonsettings.cpp",
        "vfontinstaller.cpp",
        "vfontinstaller.h",
        "vvalentinasettings.cpp",
        "commandoptions.cpp",
        "qxtcsvmodel.cpp",
        "vtablesearch.cpp",
        "literals.cpp",
        "vmodifierkey.cpp",
        "compatibility.h",
        "lambdaconstants.h",
        "def.h",
        "testpath.h",
        "vabstractvalapplication.h",
        "vmath.h",
        "vabstractapplication.h",
        "projectversion.h",
        "vcommonsettings.h",
        "vvalentinasettings.h",
        "debugbreak.h",
        "vlockguard.h",
        "vsysexits.h",
        "commandoptions.h",
        "qxtcsvmodel.h",
        "vtablesearch.h",
        "customevents.h",
        "defglobal.h",
        "testvapplication.h",
        "literals.h",
        "vmodifierkey.h",
        "typedef.h",
        "vabstractshortcutmanager.h",
        "vabstractshortcutmanager.cpp",
        "vtranslator.h",
        "vtranslator.cpp",
        "vmainbase.h",
    ]

    Group {
        name: "AppImage"
        condition: buildconfig.enableAppImage && qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        files: [
            "appimage.h",
            "appimage.cpp",
        ]
    }

    Group {
        name: "dialogs"
        prefix: "dialogs/"
        files: [
            "dialogaskcollectstatistic.cpp",
            "dialogaskcollectstatistic.h",
            "dialogaskcollectstatistic.ui",
            "dialogexporttocsv.cpp",
            "dialogselectlanguage.cpp",
            "dialogexporttocsv.h",
            "dialogselectlanguage.h",
            "dialogexporttocsv.ui",
            "dialogselectlanguage.ui",
            "dialogselectmeasurementstype.cpp",
            "dialogselectmeasurementstype.h",
            "dialogselectmeasurementstype.ui",
            "vshortcutdialog.cpp",
            "vshortcutdialog.h",
            "vshortcutdialog.ui",
        ]
    }

    Group {
        name: "svgfont"
        prefix: "svgfont/"
        files: [
            "svgdef.cpp",
            "svgdef.h",
            "vsvgfont.cpp",
            "vsvgfont.h",
            "vsvgfont_p.h",
            "vsvgfontdatabase.cpp",
            "vsvgfontdatabase.h",
            "vsvgfontengine.cpp",
            "vsvgfontengine.h",
            "vsvgfontengine_p.h",
            "vsvgfontreader.cpp",
            "vsvgfontreader.h",
            "vsvgglyph.cpp",
            "vsvgglyph.h",
            "vsvgglyph_p.h",
            "vsvghandler.cpp",
            "vsvghandler.h",
            "vsvgpathtokenizer.cpp",
            "vsvgpathtokenizer.h",
        ]
    }

    // theme/ extrait vers VThemeLib

    Group {
        name: "macOS native window theme"
        condition: qbs.targetOS.contains("macos")
        files: [
            "macwindowtheme.h",
            "macwindowtheme.mm",
        ]
    }

    Group {
        name: "Codecs: Text codec (internal)"
        condition: Utilities.versionCompare(Qt.core.version, "6") >= 0 && !project.withTextCodec
        files: [
            "vtextcodec.cpp",
            "vtextcodec.h"
        ]
    }

    Group {
        name: "Resources"
        prefix: "share/resources/"
        files: [
            "flags.qrc",
            "scenestyle.qrc",
            "cursor.qrc",
        ]
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["printsupport", "widgets"] }
        Depends { name: "VExceptionLib" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
