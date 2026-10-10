/************************************************************************
 **
 **  @file   otoolpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The pleat tool: the folds taken on their departure lines, and held by their own figure.
 **
 **          A pleat is fabric folded back on itself. The user gives the departure lines — two points each, as a
 **          slash-and-spread is given its cuts — the objects that travel, and the value each pleat shows once
 **          folded; everything else is derived, and the one thing that is never typed is the axis of the Move,
 **          because a move that is not perpendicular to a departure line does not send that line back onto its own
 **          copy. A double pleat is two pleats of the same value side by side, the second one reversed: the object
 **          carries one rule per departure line, and no special case.
 **
 **          The operation is a translation of the objects that travel, plus a figure it draws and owns: the
 **          departure lines and their copies, the fold lines, and the cap of each end (see OPleatDisplay). The
 **          intermediate points the manual construction needed — the middles, the cap points, the tangent point —
 **          never become objects the user has to maintain; they are recomputed on every parse.
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

#ifndef OTOOLPLEAT_H
#define OTOOLPLEAT_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "opleatdisplay.h"
#include "opleatrule.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class QDomElement;
class QGraphicsSceneContextMenuEvent;
class VFormula;

struct OToolPleatInitData : VAbstractOperationInitData
{
    /** The departure lines, resolved: the caller that reads a file only says which points they are. */
    QVector<OPleatDeparture> departures{};

    /** Whether the fold line of every pleat is drawn: it joins the two caps of a pleat, so it only means something
     *  where both of them are. */
    bool foldLine{true};

    OPleatDisplay display{};
};

class OToolPleat : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~OToolPleat() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolPleat *;
    static auto Create(OToolPleatInitData &initData) -> OToolPleat *;

    /** @brief Read back what SavePleat wrote. Only says which points the departure lines are made of: where they
     *  are, and which way the edge runs there, are read again from the container by every parse. */
    static void ExtractPleat(const QDomElement &domElement, OToolPleatInitData &initData);

    /** @brief Write the departure lines of the operation as their own element, one item per line — the very shape a
     *  recipe step holds, so that a template replays the pleats it was built from. */
    static void SaveDepartures(VAbstractPattern *doc, QDomElement &tag, const QVector<OPleatDeparture> &departures);

    static const QString ToolType;
    static const QString TagPleats;
    static const QString AttrFirstPoint;
    static const QString AttrSecondPoint;
    static const QString AttrValue;
    static const QString AttrKind;
    static const QString AttrKindSimple;
    static const QString AttrKindDouble;
    static const QString AttrCap1;
    static const QString AttrCap2;
    static const QString AttrCap1Sense;
    static const QString AttrCap2Sense;
    static const QString AttrCap1Ref;
    static const QString AttrCap2Ref;
    static const QString AttrNotch1;
    static const QString AttrNotch2;
    static const QString AttrFoldLine;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Pleat)
    };

    /** @brief The departure lines, as the operation holds them: which points they are, what the user asked for at
     *  each end, and the value and the kind of the pleat taken on each of them. Where the ends are, and which way the
     *  edge runs there, are recomputed by every parse. */
    auto Departures() const -> const QVector<OPleatDeparture> & { return m_departures; }

    auto IsFoldLineShown() const -> bool { return m_foldLine; }
    void SetFoldLineShown(bool show);

    /** @brief Whether some pleat can draw its fold line at all: the line joins the two caps of a pleat, and the dock
     *  greys the box when no pleat has both of them. */
    auto FoldLinePossible() const -> bool;

    auto Display() const -> const OPleatDisplay & { return m_display; }

    /** @brief The objects of the figure another tool is built on: a point or a curve a piece, a line or any other
     *  tool reads by its id.
     *
     *  Such an object cannot be taken out of the pattern: the box that carries it stays grey in the dialog, exactly
     *  as the Delete action refuses a tool that has dependents. It is the figure's points and curves that are asked,
     *  never the copies — a copy belongs to the operation and goes away with it (see IsRemovable). */
    auto FigureObjectsInUse() const -> QSet<quint32>;

    auto DisplayLineType() const -> QString { return m_display.lineTypeLine; }
    void SetDisplayLineType(const QString &typeLine);

    auto DisplayLineColor() const -> QString { return m_display.lineColor; }
    void SetDisplayLineColor(const QString &color);

    auto DisplayFoldLineType() const -> QString { return m_display.foldTypeLine; }
    void SetDisplayFoldLineType(const QString &typeLine);

    auto DisplayFoldLineColor() const -> QString { return m_display.foldColor; }
    void SetDisplayFoldLineColor(const QString &color);

    void ShowVisualization(bool show) override;

    /** @brief Open the options dialog, exactly as the context menu's "Options" entry does. The dock offers a button
     *  for it: the departure lines are pairs of points, and the dock has no editor for one. */
    void OpenOptionsDialog();

    /** @brief The departure line of the first pleat is the fold the whole operation is taken on: a layer must leave
     *  its two points where they are, whenever the layer is created. */
    auto KeptInPlace() const -> QSet<quint32> override;

public slots:
    /** @brief Give the figure the items of the objects the parse has just created, and take away those of the
     *  objects it no longer holds.
     *
     *  A lite parse creates the objects of the figure in the container and never removes one — only a full parse
     *  empties it — so the scene follows the *figure*, never what the container happens to hold: what the file no
     *  longer names loses its item, and what it names gains one. */
    void FullUpdateFromFile() override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void RefreshDataInFile() override;

    /** @brief Where the label of a point the FIGURE made stands, kept in the entry of the figure that holds it.
     *
     *  The base writes the container object — what the scene draws — and the `<destination>` record of the point that
     *  moved. A point of the figure is no destination: the record is not found, nothing is written, and the next parse
     *  puts the label back at the place the point of the departure line carries. Kept in the entry itself, exactly as
     *  OToolDartCap keeps the place of its own cap. */
    void ChangeLabelPosition(quint32 id, const QPointF &pos) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    auto ExtraPointIds() const -> QVector<quint32> override;
    auto ExtraCurveIds() const -> QVector<quint32> override;
    auto ExtraLineIds() const -> QVector<quint32> override;

    auto FigureFollowsLiteParse(const QDomElement &oldElement, const QDomElement &newElement) const -> bool override;

    auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32> override;

private:
    Q_DISABLE_COPY_MOVE(OToolPleat) // NOLINT

    QVector<OPleatDeparture> m_departures{};
    bool m_foldLine{true};
    OPleatDisplay m_display{};

    /** @brief The two points of the departure line of the first pleat: the fold the whole operation is taken on, and
     *  the pair a layer must leave where they are (see the override of KeptInPlace).
     *
     *  Written once, because the creation and the override answer the same question — they were written twice, and the
     *  spec keeps the defect it cost: a layer ticked after the fact did not follow the exclusion on one of the two. */
    static auto KeptInPlace(const QVector<OPleatDeparture> &departures) -> QSet<quint32>;

    explicit OToolPleat(const OToolPleatInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Write the options of the pleats themselves — their departure lines, their values and their boxes, and
     *  the fold line — on the operation element. The display is written beside them by its own Save. */
    void SavePleat(QDomElement &tag) const;

    /** @brief Keep in @a m_display the place a point of the figure was dragged to. False when @a point is not one of
     *  the points the figure made — the base has then already written it, or there is nothing to write. */
    auto WriteFigureLabelPlace(quint32 point, const QPointF &pos) -> bool;

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;

    /** @brief Give the display the ids its objects still miss: its points, its lines and its caps. */
    void SyncDisplay(const QVector<SourceItem> &source);

    /** @brief Put the objects of the figure in the active layer.
     *
     *  A layer names its objects by id, and what a tool produces is attached to one when it is created — but a
     *  figure the operation draws *afterwards* (a pleat added, a fold line turned on, a cap that became oblique)
     *  would belong to no layer at all, and the eye of every layer would leave it on the canvas. The ids are read
     *  from the figure itself, so the call does not depend on the file having been written yet. */
    void AttachFigureToLayer();

    /** @brief Drop the items of the objects the figure no longer holds, and give one to the objects it holds now.
     *
     *  The two are one question — what the operation draws, read of its own figure — and the answer is what lets a
     *  change of that figure be applied without a full parse (see FigureFollowsLiteParse). A line is never dropped
     *  here: it is a tool of the pattern, which only a full parse builds and only a full parse takes away.
     *
     *  @return true when an object was given its item, false when the scene already showed the whole figure. */
    auto SyncFigureItems() -> bool;

    /** @brief Keep the two halves of every notch off the canvas.
     *
     *  They are objects of the pattern — a piece is built through the notch, so the edge has to be cut in two there —
     *  and they are not what the pattern shows: the edge draws itself whole, exactly as a cut leaves its parent
     *  curve alone while its segments are turned off. A curve answers the layer as a whole, so the eye of a layer
     *  can never take them away: the operation takes them away itself, on every parse. */
    void HideNotchHalves();

    /** @brief Write this tool's element as an option change does, and ask for the parse the change needs.
     *
     *  A change of the figure's pens is a lite parse, exactly as for the display of a slash-and-spread; one that
     *  moves a point, creates a line or takes one away needs a full parse, in a group of its own. */
    void SaveOptionsAndReparse(bool fullParse, const QString &commandText);
};

#endif // OTOOLPLEAT_H
