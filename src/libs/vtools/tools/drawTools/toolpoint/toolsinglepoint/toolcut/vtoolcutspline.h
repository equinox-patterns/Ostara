/************************************************************************
 **
 **  @file   vtoolcutspline.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 12, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VTOOLCUTSPLINE_H
#define VTOOLCUTSPLINE_H


#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vtoolcut.h"

class VAbstractCubicBezier;
class VPointF;
class VSpline;
template <class T> class QSharedPointer;

/**
 * @brief The VToolCutSpline class for tool CutSpline. This tool find point on spline and cut spline on two.
 */
class VToolCutSpline : public VToolCut
{
    Q_OBJECT // NOLINT
public:
    // Where the cut point lands, once the length has been measured along the curve.
    enum class CutSide
    {
        Inside,     // on the curve: the point ends the first sub-curve and starts the second
        BeyondEnd,  // past the end: the curve is prolonged, and the point ends the second sub-curve
        BeforeStart // before the start: the curve is prolonged, and the point starts the first sub-curve
    };

    // The two sub-curves a cut produces, as control points. When the cut point falls outside the
    // curve, one of the sub-curves is the piece prolonging the curve up to that point.
    struct SubCurves
    {
        QPointF firstP1{};
        QPointF firstP2{};
        QPointF firstP3{};
        QPointF firstP4{};
        QPointF secondP1{};
        QPointF secondP2{};
        QPointF secondP3{};
        QPointF secondP4{};
        QPointF cutPoint{};
        CutSide side{CutSide::Inside};
    };

    // Splits a curve at a length measured from its start. A negative length is measured from its
    // end. Either way, a length falling outside the curve places the cut point on the curve's
    // prolongation, which becomes one of the two sub-curves.
    static auto SplitCurve(qreal length, const QSharedPointer<VAbstractCubicBezier> &spl) -> SubCurves;

    // Builds the two sub-curves, giving their shared endpoint the final cut point (name, label).
    static void BuildSubCurves(const SubCurves &curves,
                               const QSharedPointer<VAbstractCubicBezier> &spl,
                               const VPointF &cutPoint,
                               VSpline &first,
                               VSpline &second);

    ~VToolCutSpline() override =default;
    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolCutSpline *;
    static auto Create(VToolCutInitData &initData) -> VToolCutSpline *;
    static const QString ToolType;
    static const QString AttrSpline;
    auto type() const -> int override {return Type;}
    enum { Type = UserType + static_cast<int>(Tool::CutSpline)};
    void  ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id=NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(VToolCutSpline) // NOLINT

    explicit VToolCutSpline(const VToolCutInitData &initData, QGraphicsItem * parent = nullptr);

    auto GatherToolChanges() const -> ToolChanges;
};

#endif // VTOOLCUTSPLINE_H
