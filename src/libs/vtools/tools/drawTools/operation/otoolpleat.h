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
    auto DepartureCount() const -> int { return static_cast<int>(m_departures.size()); }

    /** @brief How many pleats the departure lines make: one per simple line, two per double one. */
    auto PleatCount() const -> int;

    auto IsFoldLineShown() const -> bool { return m_foldLine; }
    void SetFoldLineShown(bool show);

    /** @brief Whether some pleat can draw its fold line at all: the line joins the two caps of a pleat, and the dock
     *  greys the box when no pleat has both of them. */
    auto FoldLinePossible() const -> bool;

    auto Display() const -> const OPleatDisplay & { return m_display; }
    void SetDisplay(const OPleatDisplay &display);

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

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void RefreshDataInFile() override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

    auto ExtraPointIds() const -> QVector<quint32> override;
    auto ExtraLineIds() const -> QVector<quint32> override;

    auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32> override;

private:
    Q_DISABLE_COPY_MOVE(OToolPleat) // NOLINT

    QVector<OPleatDeparture> m_departures{};
    bool m_foldLine{true};
    OPleatDisplay m_display{};

    explicit OToolPleat(const OToolPleatInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Write the options of the pleats themselves — their departure lines, their values and their boxes, and
     *  the fold line — on the operation element. The display is written beside them by its own Save. */
    void SavePleat(QDomElement &tag) const;

    auto SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>;

    /** @brief Give the display the ids its objects still miss: its points, its lines and its caps. */
    void SyncDisplay(const QVector<SourceItem> &source);

    /** @brief Write this tool's element as an option change does, and ask for the parse the change needs.
     *
     *  A change of the figure's pens is a lite parse, exactly as for the display of a slash-and-spread; one that
     *  moves a point, creates a line or takes one away needs a full parse, in a group of its own. */
    void SaveOptionsAndReparse(bool fullParse, const QString &commandText);
};

#endif // OTOOLPLEAT_H
