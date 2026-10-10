/************************************************************************
 **
 **  @file   otoolflouncespiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The flounce spiral tool: a band of constant width whose two
 **          edges are parallel spirals, drawn in one gesture.
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

#ifndef OTOOLFLOUNCESPIRAL_H
#define OTOOLFLOUNCESPIRAL_H

#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "vabstractoperation.h"
#include "oflouncespiraldisplay.h"

class DialogTool;
class VFormula;
class VMainGraphicsScene;

struct OToolFlounceSpiralInitData : VAbstractOperationInitData
{
    quint32 startPointId{NULL_ID};   // NOLINT(misc-non-private-member-variables-in-classes)
    OFlounceSpiralDisplay display{}; // NOLINT(misc-non-private-member-variables-in-classes)
};

/**
 * @brief The flounce spiral: two parallel spirals joined at both ends, one band of constant width.
 *
 * The tool produces a whole figure at once — two curves, two segments and three points — and the figure lives in
 * a single element of the pattern file. That is deliberate, and it is what the rectangle does: an undo step and
 * a deletion are counted in elements written, not in objects drawn, so one element means the figure is created
 * in one gesture, undone in one, and deleted in one. There is no group to maintain, because there is only ever
 * one element.
 *
 * The objects themselves are objects of the container, carried by the operation: it claims them through
 * ExtraCurveIds and ExtraPointIds, which is what builds their scene items, and it keeps them as OBJECT children
 * in the pattern graph so that deleting the figure is never blocked by its own parts.
 */
class OToolFlounceSpiral : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolFlounceSpiral() override = default;
    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog,
                       VMainGraphicsScene *scene,
                       VAbstractPattern *doc,
                       VContainer *data) -> OToolFlounceSpiral *;
    static auto Create(OToolFlounceSpiralInitData &initData) -> OToolFlounceSpiral *;
    static const QString ToolType;
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::FlounceSpiral)
    };

    auto GetFormulaWidth() const -> VFormula;
    void SetFormulaWidth(const VFormula &value);

    auto GetFormulaLength() const -> VFormula;
    void SetFormulaLength(const VFormula &value);

    auto GetFormulaAngle() const -> VFormula;
    void SetFormulaAngle(const VFormula &value);

    /// The point the inner edge starts at — the one the user clicked.
    auto StartPointId() const -> quint32 { return m_startPointId; }

    auto Display() const -> const OFlounceSpiralDisplay & { return m_display; }
    void SetDisplay(const OFlounceSpiralDisplay &display);

    /// The pen and the colour of the WHOLE figure — both edges and both joining segments. One pair, not two: the
    /// flounce is a whole, and nothing on the canvas tells its parts apart. A curve answers with the very names
    /// every other curve tool uses, so the dialog and the browser read it the way they read any other curve.
    auto GetPenStyle() const -> QString;
    void SetPenStyle(const QString &penStyle);
    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &color);

    /// Name carried by the point of the given role, empty when the figure has no such point.
    auto PointName(OFlounceSpiralPoint::Role role) const -> QString;
    void SetPointName(OFlounceSpiralPoint::Role role, const QString &name);

    auto PointLabelVisible(OFlounceSpiralPoint::Role role) const -> bool;
    void SetPointLabelVisible(OFlounceSpiralPoint::Role role, bool visible);

    /** @brief The name of the ONE curve the figure offers: the outer edge, from A_2 to A_3.
     *
     *  The inner curve keeps whatever name an older pattern gave it (see OFlounceSpiralDisplay::innerAlias), but
     *  nothing offers to change it: the flounce is a whole, and a whole is named once. */
    auto OuterAlias() const -> QString;
    void SetOuterAlias(const QString &alias);

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;
    void SetVisualization() override;
    auto getTagName() const -> QString override;

    auto ExtraPointIds() const -> QVector<quint32> override;
    auto ExtraCurveIds() const -> QVector<quint32> override;
    auto ExtraLineIds() const -> QVector<quint32> override;

private:
    Q_DISABLE_COPY_MOVE(OToolFlounceSpiral) // NOLINT

    quint32 m_startPointId{NULL_ID};
    OFlounceSpiralDisplay m_display{};

    explicit OToolFlounceSpiral(const OToolFlounceSpiralInitData &initData, QGraphicsItem *parent = nullptr);

    /// Build the two edges as curves of the container, and place the three points.
    void CreateFigure(const OToolFlounceSpiralInitData &initData);

    /// Rebuild the two edges and the three points from the formulas, on an edit.
    void RefreshFigure();

    /// Give an id to both segments, and tell the base what they are made of.
    void EnsureLines();

    /// Restate the figure in <source> and <destination>, the two elements the schema imposes.
    void SyncSourceDestination();

    /** @brief Put the objects of the figure into the active layer.
     *
     *  The same rule as the pleat (OToolPleat::AttachFigureToLayer): a layer names its objects, and the pairs
     *  follow one contract — a point answers to the operation that carries it, a curve and a line to their own id.
     *  Without this call the figure of this tool was written into no layer at all. */
    void AttachFigureToLayer();

    /// Rename the point of a role, and write the new name on the point the container holds. Silent: the caller
    /// either writes the element already (SaveDialog) or hands over a whole figure (SetDisplay). The public
    /// setter writes the element as well, one edit at a time.
    void ApplyPointName(OFlounceSpiralPoint::Role role, const QString &name);

    /// Show or hide the name of a point on the object the container holds, and nowhere else: the caller is writing
    /// the element already (SaveDialog). Every other path goes through the public setter, which asks the base to
    /// write all four places at once.
    void ApplyPointLabelVisible(OFlounceSpiralPoint::Role role, bool visible);

    /// Write the figure into the document, the way every other option of the tool is written. An operation owns no
    /// object in the container, so the fake one is what SaveOption expects — the reason every setter of this file
    /// goes through it.
    void PersistFigure();

    /** @brief Whether @a id is one of the objects the operation writes in <destination>.
     *
     *  On a parse a destination is read back from the file, and the objects of the figure are read from it as well
     *  (ExtraPointIds/ExtraCurveIds): the same id would then be built twice, and only the last scene item of an id
     *  is kept in operatedObjects. The other one stayed painted and clickable for ever, and — worse — outside
     *  operatedObjects, so no layer could ever hide it: the figure stayed on screen with its layer hidden. */
    auto IsADestination(quint32 id) const -> bool;

    auto PointId(OFlounceSpiralPoint::Role role) const -> quint32;
};

#endif // OTOOLFLOUNCESPIRAL_H
