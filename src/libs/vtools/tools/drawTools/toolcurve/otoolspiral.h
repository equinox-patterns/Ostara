/************************************************************************
 **
 **  @file   otoolspiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Spiral tool: draws a golden spiral from an existing point,
 **          from the radius of its first quarter turn and the length
 **          wanted, and materializes the arrival as a real point.
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

#ifndef OTOOLSPIRAL_H
#define OTOOLSPIRAL_H

#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "vtoolabstractcurve.h"

class DialogTool;
class VFormula;
class VMainGraphicsScene;
class VPointF;

struct OToolSpiralInitData : VToolAbstractCurveInitData
{
    quint32 startPointId{NULL_ID};  // NOLINT(misc-non-private-member-variables-in-classes)
    QString formulaRadius{'0'};     // NOLINT(misc-non-private-member-variables-in-classes)
    QString formulaLength{'0'};     // NOLINT(misc-non-private-member-variables-in-classes)
    QString formulaAngle{'0'};      // NOLINT(misc-non-private-member-variables-in-classes)
    QString endPointName{};         // NOLINT(misc-non-private-member-variables-in-classes)
    bool endShowLabel{false};       // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 duplicate{0};           // NOLINT(misc-non-private-member-variables-in-classes)
};

/**
 * @brief The Spiral tool.
 *
 * The curve is built by OSpiral and stored as a plain VSplinePath, so everything a spline path can do — being
 * cut, being referenced by a formula, being widened by Graduated curve — it can do too.
 *
 * Its arrival point is always created: the user needs it to hang a formula on the end of the spiral, and a
 * flounce is measured from it. Only its name is the user's business.
 */
class OToolSpiral : public VToolAbstractBezier
{
    Q_OBJECT // NOLINT

public:
    ~OToolSpiral() override = default;
    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog,
                       VMainGraphicsScene *scene,
                       VAbstractPattern *doc,
                       VContainer *data) -> OToolSpiral *;
    static auto Create(OToolSpiralInitData &initData) -> OToolSpiral *;
    static const QString ToolType;
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Spiral)
    };

    auto GetFormulaRadius() const -> VFormula;
    void SetFormulaRadius(const VFormula &value);

    auto GetFormulaLength() const -> VFormula;
    void SetFormulaLength(const VFormula &value);

    auto GetFormulaAngle() const -> VFormula;
    void SetFormulaAngle(const VFormula &value);

    /// Name of the point the spiral starts at — the eye of the spiral.
    auto StartPointName() const -> QString;

    /// Name of the arrival point, the one the tool always creates.
    auto EndPointName() const -> QString;

    /// Whether the name of the arrival point is shown on canvas. It is not a creation switch: the point always
    /// exists, and this only says whether its name is drawn.
    auto EndPointLabelVisible() const -> bool;
    void SetEndPointLabelVisible(bool visible);

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;
    void SetVisualization() override;

private:
    Q_DISABLE_COPY_MOVE(OToolSpiral) // NOLINT

    quint32 m_startPointId{NULL_ID};
    QString m_formulaRadius;
    QString m_formulaLength;
    QString m_formulaAngle;

    explicit OToolSpiral(const OToolSpiralInitData &initData, QGraphicsItem *parent = nullptr);

    auto GatherToolChanges() const -> ToolChanges;
};

#endif // OTOOLSPIRAL_H
