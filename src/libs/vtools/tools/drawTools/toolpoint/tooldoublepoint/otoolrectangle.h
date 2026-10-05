/************************************************************************
 **
 **  @file   otoolrectangle.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  a rectangle: three points and four sides, from a base point
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

#ifndef OTOOLRECTANGLE_H
#define OTOOLRECTANGLE_H

#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vtooldoublepoint.h"

class VFormula;
class VSimplePoint;
class VScaledLine;
class QGraphicsSceneContextMenuEvent;

template <class T> class QSharedPointer;

struct OToolRectangleInitData : VDrawToolInitData
{
    quint32 basePointId{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes) — the existing point A
    QString formulaWidth{};       // NOLINT(misc-non-private-member-variables-in-classes)
    QString formulaHeight{};      // NOLINT(misc-non-private-member-variables-in-classes)

    // The three points the tool creates. B and C ride the base class; D is carried here.
    quint32 p2id{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes) — B
    quint32 p3id{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes) — C
    quint32 p4id{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes) — D

    QString name2{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString name3{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString name4{}; // NOLINT(misc-non-private-member-variables-in-classes)

    qreal mx2{labelMX}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal my2{labelMY}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal mx3{labelMX}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal my3{labelMY}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal mx4{labelMX}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal my4{labelMY}; // NOLINT(misc-non-private-member-variables-in-classes)

    // The name of a created point is not shown unless the user asks for it — the window offers one box per
    // point, and Ostara shows no point name by default.
    bool showLabel2{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool showLabel3{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool showLabel4{false}; // NOLINT(misc-non-private-member-variables-in-classes)

    // The two lengths may be named, and a named length is a variable the rest of the pattern can use.
    QString widthLineRoleName{};  // NOLINT(misc-non-private-member-variables-in-classes)
    QString heightLineRoleName{}; // NOLINT(misc-non-private-member-variables-in-classes)

    QString typeLine{TypeLineLine}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString lineColor{ColorBlack};  // NOLINT(misc-non-private-member-variables-in-classes)
};

/**
 * @brief A rectangle, as a pattern object: its base point, its three created points, and its four sides.
 *
 * The construction itself lives in ORectangleRule, shared with the window, the preview and the tool, so that
 * the four cannot disagree about what a rectangle is. What this class adds is the rectangle as a pattern
 * object — the points in the container, the four sides drawn between them, and how all of that is read and
 * written.
 *
 * The width and the height are read once, when the rectangle is created: the points keep the place they were
 * given, so the rectangle is not re-driven by its formulas at every reload. The formulas are still written to
 * the file, exactly as they were asked for, so that reopening the window of the tool shows what was typed.
 */
class OToolRectangle : public VToolDoublePoint
{
    Q_OBJECT // NOLINT

public:
    ~OToolRectangle() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolRectangle *;
    static auto Create(OToolRectangleInitData initData) -> OToolRectangle *;

    static const QString ToolType;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Rectangle)
    };

    /** @brief The tool's own label, the one the recipe and the dependency tree write a step under. A
     *  rectangle creates three points and has no point of its own, so unlike every other tool of this family
     *  it carries that label here rather than borrowing a point's name. */
    auto name() const -> QString { return BasePointName(); }

    void ShowVisualization(bool show) override;

    auto GetFormulaWidth() const -> QString;
    auto GetFormulaHeight() const -> QString;
    auto GetFormulaWidthF() const -> VFormula;
    auto GetFormulaHeightF() const -> VFormula;
    /** @brief Name a new width, or a new height. Both ends of the tool are the same gesture: put the two lengths
     *  into the container — the gesture `Create()` already performs on every parse — then write them to the file. */
    void SetFormulaWidth(const VFormula &value);
    void SetFormulaHeight(const VFormula &value);

    auto GetBasePointId() const -> quint32 { return m_basePointId; }
    auto BasePointName() const -> QString;

    auto GetLineColor() const -> QString { return m_lineColor; }
    void SetLineColor(const QString &value);

    /** @brief The line type and colour are the ones the window offers, and the four sides share them. The
     *  type itself lives in VDrawTool; only the colour is ours. */
    void SetLineType(const QString &value) override;

    auto nameP4() const -> QString;
    void setNameP4(const QString &name);
    auto GetP4Id() const -> quint32 { return m_p4id; }

    /** @brief The four sides of the rectangle, base point → B → C → D → base point: the very pairs the tool
     *  registers in the container, so the same order the Line_ and AngleLine_ variables are named in. They are
     *  drawn by this tool alone — there is no line tool behind them — and without this declaration no operation
     *  could transform them. */
    auto DrawnSegments() const -> QVector<VDrawnSegment> override
    {
        return {{m_basePointId, p1id, getLineType(), GetLineColor()},
                {p1id, p2id, getLineType(), GetLineColor()},
                {p2id, m_p4id, getLineType(), GetLineColor()},
                {m_p4id, m_basePointId, getLineType(), GetLineColor()}};
    }

    /** @brief The four items that draw those sides, in the same order: a click on one picks that side alone. */
    auto DrawnSegmentItems() const -> QVector<VScaledLine *> override
    {
        return {m_lineAB, m_lineBC, m_lineCD, m_lineDA};
    }

    /** @brief The three created points, reachable by the dock, which names them one by one. */
    auto GetP1Id() const -> quint32 { return p1id; }
    auto GetP2Id() const -> quint32 { return p2id; }

    /** @brief D is a fourth point the base class knows nothing about: it only ever looks at p1id and p2id, and
     *  answers `false` / does nothing for anything else. Without these two overrides the "show the name" box of
     *  D could never stay checked — the dock would ask, get `false`, and uncheck it again. */
    auto IsLabelVisible(quint32 id) const -> bool override;
    void SetLabelVisible(quint32 id, bool visible) override;

    /** @brief D carries a name of its own, and that name is moved like any other. The base class remembers only
     *  p1id and p2id, so without this override the drag of D's name writes nothing at all — neither the model nor
     *  the file — and the label comes back at the next parse. The attribute pair is the one SaveOptions already
     *  uses for D: mx3/my3. */
    void ChangeLabelPosition(quint32 id, const QPointF &pos) override;

    /** @brief D is a fourth point the base class does not carry, and hiding the layer that holds it must hide
     *  it as well: without this override a hidden layer would take B and C away and leave D alone on the canvas. */
    void GroupVisibility(quint32 object, bool visible) override;

    /** @brief The two lengths can be given a name of their own, and that name becomes a variable the rest of
     *  the pattern can use — `Line_<name>`. The width is the top side (base point → B), the height is the
     *  right one (B → C): the very two sides the pastilles of the window name. */
    auto GetWidthLineRoleName() const -> QString;
    void SetWidthLineRoleName(const QString &suffix);

    auto GetHeightLineRoleName() const -> QString;
    void SetHeightLineRoleName(const QString &suffix);

public slots:
    void Enable() override;
    void FullUpdateFromFile() override;
    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    /** @brief The selection highlight of the four sides, through the helper every other line of the canvas
     *  uses. Without it a selected rectangle shows no selection at all. */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(OToolRectangle) // NOLINT

    quint32 m_basePointId{NULL_ID};
    QString m_formulaWidth{};
    QString m_formulaHeight{};
    quint32 m_p4id{NULL_ID};

    /** @brief D is not one of the two points the base class carries, so it is held here — created, moved,
     *  renamed, hovered and selected like the other two. */
    VSimplePoint *m_fourthPoint{nullptr};

    /** @brief The four sides, as children of the tool. They own their own geometry invalidation: setLine()
     *  calls prepareGeometryChange(), which is what keeps a moved base point from leaving a trail. */
    VScaledLine *m_lineAB{nullptr};
    VScaledLine *m_lineBC{nullptr};
    VScaledLine *m_lineCD{nullptr};
    VScaledLine *m_lineDA{nullptr};

    QString m_lineColor{ColorBlack};

    explicit OToolRectangle(const OToolRectangleInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Put the two lengths into the container: the three corners are recomputed from the formulas by the
     *  very rule the creation uses, and the three points are written back with the ids they already have. This is
     *  what a changed width or height has to do before anything is saved — the rest of the pattern, starting with
     *  the line variables the sides are named by, is read from these positions. */
    void UpdateLengths();

    /** @brief Write the tool element through SaveOption() — the same gesture every other tool makes from the
     *  property browser, undo command included — then rebuild the geometry from the new lengths. */
    void SaveOptionAndRebuild();

    /** @brief The key a named side is stored under: the two points that close it, in the direction it is read.
     *  Same shape as every other named line of a pattern. */
    auto WidthPairKey() const -> QString;
    auto HeightPairKey() const -> QString;
    void SetLineRoleNameForPair(const QString &pairKey, const QString &suffix);

    /** @brief The tooltip of a created point: its label, then the two sides it closes, with their length and
     *  angle. The rectangle is read as its four sides, so a corner is only meaningful with its two. */
    auto PointToolTip(quint32 id) const -> QString;

    /** @brief Put the four sides where the four points now are. */
    void RefreshGeometry();

    /** @brief Put the chosen type and colour on the four sides.
     *
     * Built exactly like the single line of EndLine — same child item, same pen helpers, same geometry, same
     * highlight — so a side of a rectangle behaves like every other line of the canvas. */
    void ApplyPenToSides();

    /** @brief The two sides that are the width and the height, and the two that are neither. The first two
     *  carry their colour while the rectangle is selected; outside of a selection all four are alike. */
    auto DimensionLines() const -> QVector<VScaledLine *>;

    /** @brief Light the two lengths, or put them out, according to whether the rectangle is selected. */
    void UpdateDimensionHighlight();
    void UpdateDimensionHighlight(bool selected);
};

#endif // OTOOLRECTANGLE_H
