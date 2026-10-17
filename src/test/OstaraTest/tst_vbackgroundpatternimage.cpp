/************************************************************************
 **
 **  @file   tst_vbackgroundpatternimage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Deux défauts upstream que ce fichier tient :
 **
 **          1. un SVG qui déclare un canvas démesuré passait `IsValid()` — `QSvgRenderer` n'a aucun
 **             garde-fou — puis `QGraphicsItem::DeviceCoordinateCache` demandait un pixmap de cette taille
 **             et l'application tombait au fond de `QRasterPaintEngine` (`cb6347f62`) ;
 **          2. un buffer `<svg>` sans nom de fichier pouvait être résolu en `application/xml` par la base
 **             mimetype, et la comparaison d'alias refusait alors un SVG parfaitement valide (`9951128c3`).
 **
 **          Les cas sont testés dans les deux branches d'`IsValid()`, parce que ce sont deux chemins de code
 **          distincts : image **liée** par un chemin (`FromFile(..., false)`) et image **intégrée** en base64
 **          (`FromFile(..., true)`). Un correctif posé dans une seule branche laisse l'autre ouverte.
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

#include "tst_vbackgroundpatternimage.h"

#include "xml/vbackgroundpatternimage.h"

#include <QtTest>

#include <QFile>
#include <QGuiApplication>
#include <QTextStream>

namespace
{
// Un canvas de 200 000 × 200 000 pixels : au-delà du plafond de 256 Mo / 4 pixels que `IsValid()` applique.
constexpr int oversizedSide = 200000;

auto SvgWithCanvas(int width, int height) -> QByteArray
{
    return QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="%1" height="%2" viewBox="0 0 10 10">
  <rect x="0" y="0" width="10" height="10" fill="black"/>
</svg>)")
            .arg(width)
            .arg(height)
            .toUtf8();
}

auto MalformedSvg() -> QByteArray
{
    return QByteArrayLiteral(
        R"(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"><rect x="0" y="0"</svg>)");
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_BackgroundPatternImage::TST_BackgroundPatternImage(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::initTestCase()
{
    QVERIFY2(m_dir.isValid(), "Impossible de créer le dossier temporaire des images de test.");
}

//---------------------------------------------------------------------------------------------------------------------
auto TST_BackgroundPatternImage::WriteFile(const QString &name, const QByteArray &content) -> QString
{
    const QString path = m_dir.filePath(name);

    QFile file(path);
    if (not file.open(QIODevice::WriteOnly))
    {
        return {};
    }

    file.write(content);
    file.close();

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::SvgOnANormalCanvasIsAccepted()
{
    const QString path = WriteFile(QStringLiteral("normal.svg"), SvgWithCanvas(10, 10));
    QVERIFY(not path.isEmpty());

    const VBackgroundPatternImage linked = VBackgroundPatternImage::FromFile(path, false);
    QVERIFY2(linked.IsValid(), qUtf8Printable(linked.ErrorString()));

    const VBackgroundPatternImage embedded = VBackgroundPatternImage::FromFile(path, true);
    QVERIFY2(embedded.IsValid(), qUtf8Printable(embedded.ErrorString()));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::OversizedSvgIsRejectedWhenLinked()
{
    const QString path = WriteFile(QStringLiteral("oversized.svg"), SvgWithCanvas(oversizedSide, oversizedSide));
    QVERIFY(not path.isEmpty());

    const VBackgroundPatternImage image = VBackgroundPatternImage::FromFile(path, false);
    QVERIFY(not image.IsValid());
    QVERIFY(image.ErrorString().contains(QStringLiteral("canvas size")));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::OversizedSvgIsRejectedWhenEmbedded()
{
    const QString path = WriteFile(QStringLiteral("oversized_embedded.svg"),
                                   SvgWithCanvas(oversizedSide, oversizedSide));
    QVERIFY(not path.isEmpty());

    const VBackgroundPatternImage image = VBackgroundPatternImage::FromFile(path, true);
    QVERIFY(not image.IsValid());
    QVERIFY(image.ErrorString().contains(QStringLiteral("canvas size")));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::MalformedEmbeddedSvgIsRejected()
{
    const QString path = WriteFile(QStringLiteral("malformed.svg"), MalformedSvg());
    QVERIFY(not path.isEmpty());

    // Une image intégrée déclarée image/svg+xml est validée par son renderer : c'est la règle posée par
    // 9951128c3, qui ne se fie plus à la comparaison d'alias de la base mimetype.
    const VBackgroundPatternImage image = VBackgroundPatternImage::FromFile(path, true);
    QVERIFY(not image.IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_BackgroundPatternImage::ValidityCacheIsInvalidated()
{
    const QString path = WriteFile(QStringLiteral("cached.svg"), SvgWithCanvas(10, 10));
    QVERIFY(not path.isEmpty());

    VBackgroundPatternImage image = VBackgroundPatternImage::FromFile(path, false);
    QVERIFY2(image.IsValid(), qUtf8Printable(image.ErrorString()));

    // `IsValid()` is cached (it runs on every hover-move). Replacing the content must drop that cache, or the
    // image stays valid forever and the new bytes are never looked at.
    image.SetContentData(QByteArrayLiteral("not an image at all"), QStringLiteral("image/png"));
    QVERIFY(not image.IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    TST_BackgroundPatternImage tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_vbackgroundpatternimage.moc"
