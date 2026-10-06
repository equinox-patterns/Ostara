/************************************************************************
 **
 **  @file   otoolpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The pleat tool: one fold, taken on a prise, and held by its own figure.
 **
 **          A pleat is fabric folded back on itself. The user gives the prise — two points — the objects that
 **          travel, and the value the pleat shows once folded; everything else is derived, and the one thing that
 **          is never typed is the axis of the Move, because a move that is not perpendicular to the prise does not
 **          send the prise back onto its own copy. A double pleat is two pleats of the same value side by side,
 **          the second one reversed: the object carries one rule, run twice, and no special case.
 **
 **          The operation is a translation of the objects that travel, plus a figure it draws and owns: the prise,
 **          its copies, the fold line, and the cap of each end (see OPleatDisplay). The intermediate points the
 **          manual construction needed — the middles, the cap points, the tangent point — never become objects the
 **          user has to maintain; they are recomputed on every parse.
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
    /** The two ends of the prise, resolved: the caller that reads a file only says which points they are. */
    OPleatEnd first{};
    OPleatEnd second{};

    /** The value of the pleat as the user typed it: a formula, in the pattern's unit, read once per parse. */
    QString value{};

    OPleatKind kind{OPleatKind::Simple};

    /** Whether the fold line itself is drawn: it joins the two caps, so it only means something when both are. */
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

    /** @brief Read back what SavePleat wrote. Only says which points the prise is made of: where they are, and
     *  which way the edge runs there, are read again from the container by every parse. */
    static void ExtractPleat(const QDomElement &domElement, OToolPleatInitData &initData);

    static const QString ToolType;
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

    /** @brief The two ends of the prise, as the operation holds them: which points they are, what the user asked
     *  for there, and where the edge runs — the last one recomputed by every parse. */
    auto First() const -> const OPleatEnd & { return m_first; }
    auto Second() const -> const OPleatEnd & { return m_second; }

    auto Value() const -> const QString & { return m_value; }

    /** @brief The value as the dock edits it: a formula the user can build, evaluated against the pattern. */
    auto GetFormulaValue() const -> VFormula;
    void SetFormulaValue(const VFormula &value);

    auto Kind() const -> OPleatKind { return m_kind; }
    auto IsFoldLineShown() const -> bool { return m_foldLine; }

    /** @brief Whether the fold line is actually drawn: the box says so, and the two caps have to be drawn for it to
     *  have anything to join. One question, one answer, asked by the figure and by the preview alike. */
    auto FoldLineDrawn() const -> bool { return m_foldLine && m_first.showCap && m_second.showCap; }

    /** @brief One end of the prise, by rank: 0 for the first point the user gave, 1 for the other. The options of
     *  the two ends are independent, so every one of them is asked by rank rather than by a whole struct. */
    auto End(int rank) const -> const OPleatEnd &;
    void SetEnd(int rank, const OPleatEnd &end);

    auto IsCapShown(int rank) const -> bool { return End(rank).showCap; }
    void SetCapShown(int rank, bool show);

    auto IsCapSecondSense(int rank) const -> bool { return End(rank).secondSense; }
    void SetCapSecondSense(int rank, bool second);

    auto CapEdgePoint(int rank) const -> quint32 { return End(rank).edgePointId; }
    void SetCapEdgePoint(int rank, quint32 pointId);

    void SetValue(const QString &value);
    void SetKind(OPleatKind kind);
    void SetFoldLineShown(bool show);

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
     *  for it: the prise is a pair of points, and the dock has no editor for one. */
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

    OPleatEnd m_first{};
    OPleatEnd m_second{};
    QString m_value{};
    OPleatKind m_kind{OPleatKind::Simple};
    bool m_foldLine{true};
    OPleatDisplay m_display{};

    explicit OToolPleat(const OToolPleatInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Write the options of the pleat itself — its prise, its value, its kind and its boxes — on the
     *  operation element. The display is written beside them by its own Save. */
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
