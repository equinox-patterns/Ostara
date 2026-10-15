import qbs.File
import qbs.FileInfo
import qbs.Utilities

VAppleApplicationDiskImage {
    Depends { name: "buildconfig" }
    Depends { name: "ib" }

    condition: qbs.targetOS.contains("macos") && bundleProbe.ready
    name: "Valentina DMG"
    targetName: "valentina"
    version: buildconfig.projectVersion
    builtByDefault: false
    codesign.enableCodeSigning: buildconfig.enableCodeSigning

    Probe {
        id: bundleProbe
        property string root: absoluteSourceBase
        property bool ready
        configure: {
            ready = File.exists(FileInfo.joinPaths(root, "Valentina.app"));
            found = true;
        }
    }

    Properties {
        condition: qbs.buildVariant !== "release"
        codesign.codesignFlags: ["--deep"]
        codesign.signingType: "ad-hoc"
    }

    Properties {
        condition: qbs.buildVariant === "release"
        codesign.codesignFlags: ["--options", "runtime"]
        codesign.signingType: "apple-id"
    }

    files: {
        var files = ["dist/macos/dmg/dmg.iconset"];

        // Tested on 2.1.1. At least this version doesn't crash even if the feature still doesn't work
        if (Utilities.versionCompare(qbs.version, "2.1.1") >= 0)
            files.push("dist/macos/dmg/en_US.lproj/LICENSE");

        return files;
    }

    Group {
        name: "Bundles"
        prefix: absoluteSourceBase + "/"
        files: ["Valentina.app"]
        fileTags: "dmg.input.app"
    }

    // set to false to use a solid-color background (see dmg.backgroundColor below)
    property bool useImageBackground: true
    Group {
        name: "Background image"
        condition: useImageBackground
        files: ["dist/macos/dmg/background.tiff"]
    }

    dmg.backgroundColor: "#41cd52"
    dmg.badgeVolumeIcon: false
    dmg.iconPositions: [
        {"x": 162, "y": 190, "path": "Valentina.app"},
        {"x": 162, "y": 450, "path": "Applications"},
    ]
    dmg.windowX: 420
    dmg.windowY: 250
    dmg.windowWidth: 576

    dmg.windowHeight: 606
    dmg.iconSize: 128
    dmg.format: {
        if (Qt.core.versionMajor >= 6)
            return "ULMO"; // macOS 10.15+ only

        return "ULFO"; // OS X 10.11+ only
    }
}
