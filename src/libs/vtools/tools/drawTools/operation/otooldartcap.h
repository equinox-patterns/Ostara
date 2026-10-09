/************************************************************************
 **
 **  @file   otooldartcap.h
 **  @author Sophie Denys <contact@sophiedensys.com>
 **  @date   08/10/2026
 **
 **  @brief  The cap of a dart, as an OPERATION — the shape the pleat has.
 **
 **          A dart cap is not a point: it produces a point, a crank and the sides of a cap. Written as a point
 **          tool it had to build its own scene items, reserve its own ids, persist them in the element of a
 **          `<point>` and read them back — and every one of those hand-made guarantees came apart: the sides were
 **          lost on opening, the marks and labels were put back where they started, the crank was wiped with a
 **          point, and a fresh id taken during a parse landed on objects other tools needed.
 **
 **          Every one of them is provided by VAbstractOperation, which is what the pleat is written on. So the cap
 **          is written on it too: the figure is described by an OPleatDisplay — the point C, its two sides as
 **          curve1/curve2 or side1/side2, and the crank as point/curve1/curve2 — and the framework creates,
 **          refreshes, shows, hides and persists it, parented to an item that stays at the origin.
 **
 **          The geometry does NOT change: the cap point stays the one ODartCapRule computes (the line of the edge
 **          through the opening point, met by the axis of the dart), the sides stay the ones OPleatCapRule::Sides
 **          takes, and the crank stays the one OPleatNotchRule::Place walks. The pleat's plan is deliberately not
 **          used: a dart is not a pleat whose copies travel, and OPleatRule::Compose places its caps on a line
 **          parallel to the departure line, which is not the axis of a dart.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedensys.com>
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

#ifndef OTOOLDARTCAP_H
#define OTOOLDARTCAP_H

#include <QDomElement>
#include <QPointF>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "opleatdisplay.h"
#include "vabstractoperation.h"

class DialogTool;
class QGraphicsItem;
class QGraphicsSceneContextMenuEvent;
class VMainGraphicsScene;
class VPatternGraph;

struct OToolDartCapInitData : VAbstractOperationInitData
{
    using VAbstractOperationInitData::VAbstractOperationInitData;

    /** @brief The apex of the dart, and the two points of its opening — the second very often the copy of the
     *  first, made by an operation. The sense says at which of the two the piece's edge is read. */
    quint32 apexId{NULL_ID};
    quint32 firstPointId{NULL_ID};
    quint32 secondPointId{NULL_ID};

    /** @brief A point the user designated to carry the edge, when none could be read. NULL_ID otherwise. */
    quint32 extendPointId{NULL_ID};

    bool secondSense{false};

    /** @brief The name the point C takes.
     *
     *  A dart cap written as a `<point>` had a name of its own — `C11` — and that name is the point's, not the
     *  operation's: the file kept `id`, `name`, `mx`, `my` and `showPointName` for the point the tool was. Read
     *  back here so that an already saved pattern keeps the name its author gave, and left empty on a creation
     *  from the interface, where the name is made from the apex. */
    QString pointName{};

    /** @brief Whether the name of the point C is shown, as the pattern that was saved asked.
     *
     *  A dart cap written as a `<point>` carried `showPointName`. Read back, so a converted cap shows what its
     *  author had chosen instead of what a constant happens to say. */
    bool showLabel{false};

    /** @brief Whether the two sides of the cap are drawn, and whether its crank is marked. */
    bool drawCap{true};
    bool showNotch{false};

    /** @brief The figure to write and to draw: filled from the geometry before the tool is built. */
    OPleatDisplay display{};
};

/**
 * @brief The cap of a dart, written as an operation.
 *
 * The figure is the one OPleatDisplay describes — a mark and a crank — and it is built from the geometry on every
 * parse, before the tool exists, exactly as the pleat builds its own (see OToolPleat::CreateDestination). What the
 * tool adds is what a dart has and a pleat has not: the apex, the sense that chooses the end the edge is read at,
 * and the line the cap is read on when the user designates it.
 */
class OToolDartCap : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    OToolDartCap(const OToolDartCapInitData &initData, QGraphicsItem *parent = nullptr);
    ~OToolDartCap() override = default;

    void SetDialog() override;
    void ShowVisualization(bool show) override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> OToolDartCap *;
    static auto Create(OToolDartCapInitData &initData) -> OToolDartCap *;

    static const QString ToolType;
    static const QString AttrApex;
    static const QString AttrSecondSense;
    static const QString AttrDrawCap;
    static const QString AttrNotch;

    /** @brief The figure kept as flat attributes — the shape a `<point type="dartCap">` had. Still read, so that
     *  patterns already saved open. */
    static const QString AttrCurve1;
    static const QString AttrCurve2;
    static const QString AttrNotchCurve1;
    static const QString AttrNotchCurve2;
    static const QString AttrNotchPoint;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::DartCap)
    };

    auto ApexId() const -> quint32 { return m_apexId; }
    auto FirstPointId() const -> quint32 { return m_firstPointId; }
    auto SecondPointId() const -> quint32 { return m_secondPointId; }
    auto ExtendPointId() const -> quint32 { return m_extendPointId; }
    auto IsSecondSense() const -> bool { return m_secondSense; }
    auto IsCapDrawn() const -> bool { return m_drawCap; }
    auto IsNotchDrawn() const -> bool { return m_showNotch; }

    void SetSecondSense(bool secondSense);
    void SetCapDrawn(bool drawn);
    void SetNotchDrawn(bool drawn);

    auto Display() const -> const OPleatDisplay & { return m_display; }

    /** @brief The name of the point C — which is the figure's name, and the label the recipe carries.
     *
     *  A dart cap written as a `<point>` was a point, and had its name (`C11`). The name did not move to the
     *  operation: it is still the point's, and the operation reads it from the figure it carries. */
    auto name() const -> QString;

    auto ApexName() const -> QString;
    auto FirstPointName() const -> QString;
    auto SecondPointName() const -> QString;
    auto ExtendPointName() const -> QString;

    /** @brief The pen and the colour the sides of the cap are drawn with.
     *
     *  They live in the figure — the display — which is what the operation writes and hands to the framework, so
     *  the dock reads and writes them here, exactly as it did on the point tool, where they came from
     *  VToolLinePoint. `getLineType`/`SetLineType` are the names the dock's own rows call (VDrawTool's), and the
     *  two of them are answered from the FIGURE: the pen the operation draws with is the figure's, never the
     *  tool's own — the tool's own is written nowhere and read by nobody, and letting the dock write there was
     *  what made a change of style, or of colour, do nothing at all on the canvas. */
    auto getLineType() const -> QString;
    auto GetLineColor() const -> QString { return m_display.lineColor; }
    void SetLineType(const QString &value) override;
    void SetLineColor(const QString &value);

    /** @brief Name the sources of the operation, from the objects they name.
     *
     *  A source is written in the file with a name — the schema declares it optional but NON-EMPTY — and the
     *  dialog hands the operation bare ids. The name is the object's, suffixed the way every operation names what
     *  it takes: the pleat writes `<name>__m<pleats>`, the batch move `<name>__M`, the slash-and-spread
     *  `<name>__R`; a cap that copies nothing writes `<name>__cap`. Empty names are left alone when the object is
     *  unknown, never blanked. */
    static void NameSources(QVector<SourceItem> &source, const VContainer *data);

    void OpenOptionsDialog();

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    auto ExtraPointIds() const -> QVector<quint32> override;
    auto ExtraCurveIds() const -> QVector<quint32> override;
    auto ExtraLineIds() const -> QVector<quint32> override;

    auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32> override;

    /** @brief Read the figure again on every parse.
     *
     *  A move of the curve the edge runs on changes the cap and the crank, and a lite parse alone carries that move:
     *  this is where the geometry is read again, and where the halves a crank cuts are taken off the canvas. The
     *  pleat does the same in its own FullUpdateFromFile.
     */
    void FullUpdateFromFile() override;

    /** @brief Whether the figure follows a LITE parse, or needs the whole pattern read again.
     *
     *  The pleat asks itself the same question for the same reasons: an id a lite parse registers would be gone at
     *  the next one, and a change of the objects the operation carries goes with a change of those objects.
     *  Overridden only to answer `true` where the base would answer `false` for a figure that has no lines. */
    auto FigureFollowsLiteParse(const QDomElement &oldElement, const QDomElement &newElement) const -> bool override;

    /** @brief Write the figure back into the tool's element when a parse has changed it.
     *
     *  A curve born during a parse — a side that turned out to follow the edge — carries an id the file must hold: a
     *  layer names its objects by id, and an id the file does not carry is one the next open would not know. */
    void RefreshDataInFile() override;

private:
    Q_DISABLE_COPY_MOVE(OToolDartCap) // NOLINT

    quint32 m_apexId{NULL_ID};
    quint32 m_firstPointId{NULL_ID};
    quint32 m_secondPointId{NULL_ID};
    quint32 m_extendPointId{NULL_ID};
    bool m_secondSense{false};
    bool m_drawCap{true};
    bool m_showNotch{false};
    /** @brief Whether the name of the point C is shown — kept here, because the figure is rebuilt from it and a
     *  rebuild must not lose what the file said. */
    bool m_showLabel{false};

    /** @brief The figure: the point C, its two sides, and the crank with the two halves it cuts. */
    OPleatDisplay m_display{};

    void RefreshFigure();

    /** @brief Turn the element of an old cap — a `<point>` — into the operation it became.
     *
     *  Done once, at the open, and whole: the element is renamed, the attributes an `<operation>` does not declare
     *  are taken away, and its source and its figure are written. Everything else follows from it — the tool finds
     *  its element again by the tag it declares, a read gives the figure back instead of rebuilding it from nothing,
     *  and an edit is written where the user expects it. Without it a cap written before this work could be opened
     *  and drawn, but never touched: every change went to an element nothing could find, in silence. */
    void MigrateLegacyElement();

    /** @brief Where the label of the point C stands, written where the figure keeps it.
     *
     *  The base writes the container object — what the scene draws — and the destination record of the point that
     *  moved. A dart cap copies nothing: the point C is the FIGURE's, it is no destination, and the mark of the
     *  display is what the file reads back. The old tool wrote the point's own `mx`/`my` on every drag, and without
     *  this the label came back to its old place at the next parse, and the file kept it. */
    void ChangeLabelPosition(quint32 id, const QPointF &pos) override;

    /** @brief Attach the objects of the figure to the active layer.
     *
     *  What a parse creates a moment later is named by id, and a layer must already hold those ids: a pair is an id
     *  exactly as a line's is (the pleat's own rule, see OToolPleat::AttachFigureToLayer). Without this the figure
     *  answers no layer eye, and a hidden layer leaves the cap drawn. */
    void AttachFigureToLayer();

    /** @brief Make the items of the figure the ones the display holds now.
     *
     *  What a change has added — a crank switched on, a side that took the edge — is a tool no parse was asked for,
     *  and what it has taken away must leave the canvas. The pleat asks the same question the same way
     *  (OToolPleat::SyncFigureItems); this answers which of the two happened. */
    auto SyncFigureItems() -> bool;

    /** @brief Show what the figure shows, hide what it does not.
     *
     *  Three things are never what the pattern shows: the two halves a crank cuts (a piece is built through them),
     *  the two sides of the cap when it is not drawn, and the crank with them — the box that asks for a crank is
     *  greyed when no cap is drawn. The point C stays either way: it is what the tool was, and the old tool said so
     *  in the very box that draws the sides ("the point exists either way, the two tracés are what the box adds").
     *  Re-applied after EVERY parse, because the layer pass sets every object of the operation visible again
     *  (VAbstractOperation::GroupVisibility). */
    void ApplyFigureVisibility();

    /** @brief Write the tool's options and ask for the parse that makes the canvas follow them.
     *
     *  A pen redraws lines, a flag adds or takes away objects: neither is carried by the file alone. The pleat's own
     *  helper, word for word (OToolPleat::SaveOptionsAndReparse). */
    void SaveOptionsAndReparse(bool fullParse, const QString &commandText);

    auto PointNameOf(quint32 pointId) const -> QString;
};

#endif // OTOOLDARTCAP_H
