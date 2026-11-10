/************************************************************************
 **
 **  @file   tst_folderpathsignal.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  Every library folder setter announces the move, once, with the settings key it wrote.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "tst_folderpathsignal.h"

#include "vmisc/testvapplication.h"
#include "vmisc/vcommonsettings.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// One library folder: the settings key it lives under, and the two calls the page makes on it.
struct Library
{
    const char *key;
    void (VCommonSettings::*setter)(const QString &);
    QString (VCommonSettings::*getter)() const;
};

//---------------------------------------------------------------------------------------------------------------------
/// The twelve folders of Preferences › Folders — the ones a reader can move.
auto Libraries() -> QVector<Library>
{
    return {
        {"paths/measurementLibrary", &VCommonSettings::SetPathMeasurementLibrary,
         &VCommonSettings::GetPathMeasurementLibrary},
        {"paths/pieces", &VCommonSettings::SetPathPieces, &VCommonSettings::GetPathPieces},
        {"paths/patterns", &VCommonSettings::SetPathPatterns, &VCommonSettings::GetPathPatterns},
        {"paths/collections", &VCommonSettings::SetPathCollections, &VCommonSettings::GetPathCollections},
        {"paths/clients", &VCommonSettings::SetPathClients, &VCommonSettings::GetPathClients},
        {"paths/seamAllowances", &VCommonSettings::SetPathSeamAllowances,
         &VCommonSettings::GetPathSeamAllowances},
        {"paths/symbols", &VCommonSettings::SetPathSymbols, &VCommonSettings::GetPathSymbols},
        {"paths/annotationstyles", &VCommonSettings::SetPathAnnotationStyles,
         &VCommonSettings::GetPathAnnotationStyles},
        {"paths/cartoucheTemplates", &VCommonSettings::SetPathCartoucheTemplates,
         &VCommonSettings::GetPathCartoucheTemplates},
        {"paths/pdfTemplates", &VCommonSettings::SetPathPdfTemplates, &VCommonSettings::GetPathPdfTemplates},
        {"paths/coverPageTemplates", &VCommonSettings::SetPathCoverPageTemplates,
         &VCommonSettings::GetPathCoverPageTemplates},
        {"paths/logos", &VCommonSettings::SetPathLogos, &VCommonSettings::GetPathLogos},
    };
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_FolderPathSignal::TST_FolderPathSignal(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderPathSignal::EverySetterAnnouncesItsFolder()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // Both the settings file and the folders land in the temporary directory: no test may touch the real
    // preferences of whoever runs the suite.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());

    VTestSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OstaraTest"),
                           QStringLiteral("FolderPathSignal"));
    QSignalSpy spy(&settings, &VCommonSettings::FolderPathChanged);

    const QVector<Library> libraries = Libraries();
    QCOMPARE(libraries.size(), 12);

    for (int i = 0; i < libraries.size(); ++i)
    {
        const Library library = libraries.at(i);
        const QString before = (settings.*(library.getter))();
        const QString target = tmp.filePath(QStringLiteral("library%1").arg(i));

        (settings.*(library.setter))(target);

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toString(), QString::fromLatin1(library.key));
        QCOMPARE(args.at(1).toString(), before);
        QCOMPARE(args.at(2).toString(), target);
        QCOMPARE((settings.*(library.getter))(), target);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderPathSignal::WritingTheSameFolderTwiceSaysItOnce()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());

    VTestSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OstaraTest"),
                           QStringLiteral("FolderPathSignalTwice"));
    QSignalSpy spy(&settings, &VCommonSettings::FolderPathChanged);

    const QString folder = tmp.filePath(QStringLiteral("Measurements"));
    settings.SetPathMeasurementLibrary(folder);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    // Answering the same folder again is not a move: nothing is announced, so no window rebuilds twice.
    settings.SetPathMeasurementLibrary(folder);
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(TST_FolderPathSignal)
