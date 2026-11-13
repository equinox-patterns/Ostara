/************************************************************************
 **
 **  @file   tst_placelabel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le repère interne « cercle plein » : la seule forme dont l'encre remplit le tracé.
 **
 **          Un cercle plein n'est pas une géométrie neuve — c'est le cercle, avec son intérieur peint.
 **          Ce que ces cas fixent : un seul type est rempli (la règle lue par le canevas, la mise en
 **          page, les symboles et l'écrivain SVG), le disque a exactement la géométrie du cercle, et
 **          son chemin se referme — sans quoi il n'y aurait rien à remplir.
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

#include "tst_placelabel.h"

#include "testvapplication.h"
#include "vtools/tools/nodeDetails/onodeplacelabel.h"
#include "osymbolrender.h"
#include "osymbolshapes.h"
#include "vabstractpiece.h"
#include "vgeometrydef.h"
#include "vlayoutplacelabel.h"

#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QRectF>
#include <QVector>
#include <QtTest>

namespace
{
// Every type the enumeration knows, in order: a value added tomorrow without a case here is a value the
// rule would answer "not filled" to without anyone noticing.
const QVector<PlaceLabelType> kEveryType = {
    PlaceLabelType::Segment,  PlaceLabelType::Rectangle,  PlaceLabelType::Cross,  PlaceLabelType::Tshaped,
    PlaceLabelType::Doubletree, PlaceLabelType::Corner,   PlaceLabelType::Triangle, PlaceLabelType::Hshaped,
    PlaceLabelType::Button,   PlaceLabelType::Circle,     PlaceLabelType::FilledCircle};

//---------------------------------------------------------------------------------------------------------------------
auto MakeLabel(PlaceLabelType type) -> VLayoutPlaceLabel
{
    VLayoutPlaceLabel label;
    label.SetCenter(QPointF(0, 0));
    label.SetType(type);
    label.SetRotationMatrix(QTransform());
    // A box anchored at the origin, the way a mark on a piece gives it.
    label.SetBox(QRectF(0, 0, 10, 10));
    return label;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_PlaceLabel::TST_PlaceLabel(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::OnlyTheDiscIsFilled()
{
    for (const PlaceLabelType type : kEveryType)
    {
        QCOMPARE(IsFilledPlaceLabel(type), type == PlaceLabelType::FilledCircle);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::TheDiscIsTheCircle()
{
    // The disc is the circle — same points, same order. What tells them apart is the filling, never the geometry:
    // a second circle drawn differently would be a second mark to keep in step for ever.
    const PlaceLabelImg circle = VAbstractPiece::PlaceLabelShape(MakeLabel(PlaceLabelType::Circle));
    const PlaceLabelImg disc = VAbstractPiece::PlaceLabelShape(MakeLabel(PlaceLabelType::FilledCircle));

    QCOMPARE(disc.size(), circle.size());
    for (int i = 0; i < disc.size(); ++i)
    {
        QCOMPARE(disc.at(i).size(), circle.at(i).size());
        for (int j = 0; j < disc.at(i).size(); ++j)
        {
            QCOMPARE(static_cast<QPointF>(disc.at(i).at(j)), static_cast<QPointF>(circle.at(i).at(j)));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::TheDiscCanBeFilled()
{
    const QPainterPath path =
        VAbstractPiece::LabelShapePath(VAbstractPiece::PlaceLabelShape(MakeLabel(PlaceLabelType::FilledCircle)));

    QVERIFY(not path.isEmpty());
    // A subpath that is not closed holds no surface: filling it paints nothing, and the export would carry an
    // empty disc. `contains` is the question the painter asks, so it is the one asked here.
    QVERIFY(path.contains(QPointF(0, 0)));
    QVERIFY(path.contains(QPointF(1, 1)));
    QVERIFY(not path.contains(QPointF(9, 9))); // the bounding square's corner is outside the disc
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::EveryTypeDrawsSomething()
{
    // Including the disc: a type the shape builder forgot would draw an empty mark on the canvas, in the
    // symbol library and in every export, and nothing would say so.
    for (const PlaceLabelType type : kEveryType)
    {
        const PlaceLabelImg shape = VAbstractPiece::PlaceLabelShape(MakeLabel(type));
        QVERIFY2(not shape.isEmpty(), qPrintable(QStringLiteral("no shape for type %1").arg(static_cast<int>(type))));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::TheSymbolLibraryShowsTheDiscFilled()
{
    // The symbol library and the cover page draw a mark through OSymbolRender, which paints its `fills` list and
    // only traces its `strokes`: a disc left in `strokes` alone would show as an empty ring there, next to a
    // canvas that shows it solid.
    const auto build = [](PlaceLabelType type)
    {
        const OSymbolShapes::Shape shape{OSymbolShapes::Family::PlaceLabel, static_cast<int>(type), false};
        return OSymbolRender::Build(shape);
    };

    const OSymbolRender::Drawing disc = build(PlaceLabelType::FilledCircle);
    QCOMPARE(disc.strokes.size(), 1);
    QCOMPARE(disc.fills.size(), 1);

    const OSymbolRender::Drawing ring = build(PlaceLabelType::Circle);
    QCOMPARE(ring.strokes.size(), 1);
    QVERIFY(ring.fills.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PlaceLabel::TheHandleIsClickableButPaintsNothing()
{
    // The handle lies on the mark itself: anything it drew would hide the shape the user opened the details
    // page for. It has to keep answering the scene, though — a hidden item receives no press at all — so the
    // rule is "no ink, same hit area", and that is what is checked here.
    QGraphicsScene scene;
    auto *handle = new ONodePlaceLabel();
    scene.addItem(handle);

    QImage image(40, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        QStyleOptionGraphicsItem option;
        handle->paint(&painter, &option, nullptr);
    }

    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            QCOMPARE(qAlpha(image.pixel(x, y)), 0);
        }
    }

    // Still a target, and still at the label's position: hiding the item would take the click with it.
    QVERIFY(handle->isVisible());
    QVERIFY(not handle->boundingRect().isEmpty());
    QVERIFY(handle->shape().contains(QPointF(0, 0)));
}

//---------------------------------------------------------------------------------------------------------------------
auto main(int argc, char **argv) -> int
{
    // Les couleurs des points se lisent dans une ressource compilée avec VMiscLib : sans cette ligne, un test
    // qui construit une poignée de scène la cherche en vain et laisse deux avertissements dans la sortie.
    Q_INIT_RESOURCE(scenestyle); // NOLINT
    TestVApplication const app(argc, argv);
    TST_PlaceLabel test;
    return QTest::qExec(&test, argc, argv);
}
