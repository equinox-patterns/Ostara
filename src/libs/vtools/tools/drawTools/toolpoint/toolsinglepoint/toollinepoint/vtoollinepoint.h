/************************************************************************
 **
 **  @file   vtoollinepoint.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
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

#ifndef VTOOLLINEPOINT_H
#define VTOOLLINEPOINT_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include <optional>

#include "../vgeometry/oaxis.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vformula.h"
#include "../vtoolsinglepoint.h"

template <class T> class QSharedPointer;

struct VToolLinePointInitData : VToolSinglePointInitData
{
    using VToolSinglePointInitData::VToolSinglePointInitData;

    QString typeLine{TypeLineLine};
    QString lineColor{ColorBlack};
    OAxis axis{};
};

/**
 * @brief The VToolLinePoint class parent for all tools what create point with line.
 */
class VToolLinePoint : public VToolSinglePoint
{
    Q_OBJECT // NOLINT

public:
    VToolLinePoint(VAbstractPattern *doc, VContainer *data, const quint32 &id, const QString &typeLine,
                   const QString &lineColor, const QString &formula, const quint32 &basePointId, const qreal &angle,
                   const QString &notes, QGraphicsItem *parent = nullptr);
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::LinePoint)
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    virtual auto GetFormulaLength() const -> VFormula;
    void SetFormulaLength(const VFormula &value);

    auto BasePointName() const -> QString;

    /** @brief Id of the point this tool draws its implicit line from. Used by the operation tools to
     *  transform that line together with the point. */
    auto BasePointId() const -> quint32 { return basePointId; }

    /** @brief The one segment a line point draws: towards its base point, in the order it was registered —
     *  the order the Line_ and AngleLine_ variables of the tool are named in. */
    auto DrawnSegments() const -> QVector<VDrawnSegment> override
    {
        return {{basePointId, getId(), getLineType(), GetLineColor()}};
    }

    /** @brief The item that draws that one segment, in the same order: a click on it picks the segment. */
    auto DrawnSegmentItems() const -> QVector<VScaledLine *> override { return {mainLine}; }

    auto GetAngle() const -> qreal;
    void SetAngle(const qreal &value);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    /** @brief The point holds the click (property browser): show the line it comes from, as on hover. */
    void SetActivePoint(bool active) override;

    /** @brief The layers have just been applied: the tool follows them, and so do the tracés it draws.
     *
     *  A tracé joins two points and one of them is not this tool's — the base point of a line point, the two
     *  halves a cut made of a line. A layer only reaches the objects of the tool it hides, so a tracé was left
     *  on the canvas while one of its two ends had gone: C→B stayed drawn while B was hidden. Aligned again
     *  here, once the layer pass has decided, and again on every refresh (RefreshGeometry). */
    void GroupVisibility(quint32 object, bool visible) override;

    /** @brief The axis this tool's angle is delegated to, unknown when the angle is written by hand. */
    auto GetAxis() const -> OAxis { return m_axis; }
    void SetAxis(const OAxis &axis) { m_axis = axis; }

    /**
     * @brief The angle formula the file carries — or the number the axis dictates, when there is one.
     *
     * The axis only fills the angle: what is shown, saved and measured stays a plain number, so everything that
     * reads an angle keeps working unchanged. The value is recomputed here and never cached, so a point that
     * moved after the fact never leaves a stale number on screen.
     */
    auto EffectiveAngleFormula() const -> QString;

    /** @brief Reads this tool's attributes back from the file — the axis included — so the Options dock can
     *  never show an axis an undo has taken away. */
    void RefreshAxisFromFile();

    // The axis contract (see VDrawTool). A line point offers ONE slot, its angle: the point is placed along the
    // direction the axis gives, at the distance its length formula says. Only the tools that offer it override
    // AxisSlotCount(); the implementation below is shared by them.
    auto AxisSlot(int slot) const -> OAxis override;
    void PreviewAxis(int slot, const OAxis &axis) override;
    void CommitAxis() override;
    void RevertAxis() override;

public slots:
    void Enable() override;

protected:
    /** @brief formula string with length formula. */
    QString formulaLength;

    /** @brief angle line angle. */
    qreal angle;

    /** @brief basePointId id base line point. */
    quint32 basePointId;

    /** @brief mainLine line item. */
    VScaledLine *mainLine{nullptr};

    /** @brief lineColor color of a line. */
    QString lineColor;

    /** @brief The axis of the angle, when the direction comes from two points instead of a formula. */
    OAxis m_axis{};

    /**
     * @brief The angle written by hand — what the axis replaces when it is set. Every tool that offers an
     *  axis holds its own member; the base asks for it through these two, so the shared preview, save and
     *  display code never needs to know which tool it holds.
     */
    virtual auto AngleFormulaString() const -> QString;
    virtual void SetAngleFormulaString(const QString &formula);

    /**
     * @brief Where this tool's point lands for a given angle, in pattern coordinates.
     *
     * Empty when the tool cannot place its point there — no intersection with the line or the curve it is
     * built on — and the preview then leaves the point where it stands instead of jumping to nowhere.
     */
    virtual auto PointForAxisAngle(qreal angle) const -> std::optional<QPointF>;

    void ReadToolAttributes(const QDomElement &domElement) override;

    void SaveAxisInto(QDomElement &element);

    /** @brief What the preview started from, kept only while a direction window is open. */
    QSharedPointer<VPointF> m_axisPreviewStart{};
    QString m_axisFormulaStart{};
    OAxis m_axisStart{};
    qreal m_axisAngleStart{0};
    bool m_axisPreviewing{false};

    void RefreshGeometry() override;

    /** @brief Show each tracé the tool draws only when the layers show **both** points it joins, and hide it
     *  when the segment is empty. One tracé at a time, in the order of DrawnSegments(): a tool that draws more
     *  than one — the two halves of a cut line — is judged one by one, so hiding the second point takes the
     *  second half away and leaves the first. */
    void ApplySegmentsVisibility();

    /** @brief Writes the pen the tool asks for — its one colour, its one style — on one of the lines it draws,
     *  then converges the pen so that the emphasis of a hover or of a click still reads on top of it. Used for
     *  the inherited main line and, by a tool that draws more than one line, for each of the others.
     *  @param line a line the tool draws. nullptr is ignored. */
    void RefreshAuthoredPen(VScaledLine *line);

    /** @brief Tells a line the tool draws to ask this tool for its colour — see
     *  VScaledLine::SetAuthoredColorProvider. Called for every line whose pen the tool owns. */
    void AttachAuthoredColor(VScaledLine *line);

    /** @brief Writes the emphasis of a click on every line the tool draws, at once — and takes it back when the
     *  click leaves. A hover is not a state of these lines: the tool overrides no hover event, so passing over the
     *  point leaves its tracés untouched. The colour of each line is rewritten from the tool first: the item that
     *  answers for a colour is the tool, never a neighbouring line. */
    void UpdateLinesHighlight();
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    auto MakeToolTip() const -> QString override;

    void ProcessLinePointToolOptions(const QDomElement &oldDomElement,
                                     const QDomElement &newDomElement,
                                     const QString &newLabel,
                                     const QString &newBasePointLabel);

    void SyncLineRoleNameToDom();
    void SyncLineRoleNameToDom(const QString &pairKey);

private:
    Q_DISABLE_COPY_MOVE(VToolLinePoint) // NOLINT
};

#endif // VTOOLLINEPOINT_H
