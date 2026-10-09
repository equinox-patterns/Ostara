/************************************************************************
 **
 **  @file   dialogoperationtool.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
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

#include "dialogoperationtool.h"

#include "../../tools/drawTools/operation/ooperationlines.h"
#include "../../tools/drawTools/vtoolline.h"
#include "../../tools/vabstracttool.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSharedPointer>
#include <QStringList>

#include "vlineedit.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Two line selections are the same when they name the same lines, whatever the order they were picked in. A line
// is named by the tool that carries it and the pair of its endpoints: its transformed copy's id has no say here.
auto SameLines(const QVector<OOperationLine> &first, const QVector<OOperationLine> &second) -> bool
{
    if (first.size() != second.size())
    {
        return false;
    }

    return std::all_of(first.cbegin(),
                       first.cend(),
                       [&second](const OOperationLine &line) -> bool
                       {
                           return std::any_of(second.cbegin(),
                                              second.cend(),
                                              [&line](const OOperationLine &other) -> bool
                                              {
                                                  return line.source == other.source && line.point1 == other.point1
                                                         && line.point2 == other.point2;
                                              });
                       });
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogOperationTool::DialogOperationTool(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                         QWidget *parent)
  : DialogTool(data, doc, toolId, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetVisibilityGroupName() const -> QString
{
    return (Widgets().visibilityGroupLine == nullptr) ? QString() : Widgets().visibilityGroupLine->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetVisibilityGroupName(const QString &name) const
{
    if (name.isEmpty() || Widgets().visibilityGroupLine == nullptr)
    {
        // Nothing to put in the field: it keeps the default name its own .ui gives it, which is the name of the
        // operation and is translated for it. Writing one here would label every operation "Rotation". A dialog
        // without a layer group has no field at all — the value is then simply not offered.
        return;
    }

    Widgets().visibilityGroupLine->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::HasLinkedVisibilityGroup() const -> bool
{
    return (Widgets().visibilityGroup != nullptr) && Widgets().visibilityGroup->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetHasLinkedVisibilityGroup(bool linked) const
{
    if (Widgets().visibilityGroup == nullptr)
    {
        return; // no layer group in this dialog: there is no box to check
    }

    Widgets().visibilityGroup->setChecked(linked);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetVisibilityGroupTags(const QStringList &tags) const
{
    if (Widgets().groupTags == nullptr)
    {
        return;
    }

    Widgets().groupTags->setText(tags.join(", "));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetVisibilityGroupTags() const -> QStringList
{
    return (Widgets().groupTags == nullptr) ? QStringList{} : Widgets().groupTags->text().split(',');
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetGroupCategories(const QStringList &categories)
{
    m_groupTags = categories;
    if (Widgets().groupTags != nullptr)
    {
        Widgets().groupTags->SetCompletion(m_groupTags);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetSourceObjects() const -> QVector<SourceItem>
{
    return m_sourceObjects;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetSourceObjects(const QVector<SourceItem> &value)
{
    m_sourceObjects = value;

    if (!m_sourceBaselineCaptured)
    {
        // First call populates the dialog from the tool; remember the committed object set (by object id, the one
        // the pattern holds) so later edits (add/remove) can be detected. Subsequent calls (add/remove/rename) must
        // not move the baseline.
        m_baselineSourceIds.clear();
        m_baselineSourceIds.reserve(value.size());
        for (const auto &item : value)
        {
            m_baselineSourceIds.append(item.id);
        }
        m_sourceBaselineCaptured = true;
    }

    OnSourceObjectsSet();
    CheckState(); // adding/removing objects toggles the Apply button (see CheckState)
}


//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetLineSources() const -> const QVector<OOperationLine> &
{
    return m_lineSources;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetLineSources(const QVector<OOperationLine> &value)
{
    m_lineSources = value;

    if (!m_lineBaselineCaptured)
    {
        m_baselineLineSources = value;
        m_lineBaselineCaptured = true;
    }

    OnSourceObjectsSet();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ToggleLineSource(quint32 id)
{
    if (id == NULL_ID)
    {
        return;
    }

    // A Line tool describes its own line: the descriptor is read from it, so what the operation records is the
    // pair of endpoints, exactly as for a line no tool draws.
    ToggleDrawnSegment(OOperationLines::LineDescriptor(qobject_cast<VToolLine *>(VAbstractPattern::FindTool(id))));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ToggleDrawnSegment(const OOperationLine &segment)
{
    if (segment.point1 == NULL_ID || segment.point2 == NULL_ID || segment.point1 == segment.point2)
    {
        return;
    }

    // The identity of a line is its carrier and the pair of its endpoints: one tool draws several segments under
    // its own id — four sides of a rectangle, two tracés of a dart cap — and the pair is what tells them apart.
    const auto sameSegment = [&segment](const OOperationLine &line) -> bool
    {
        return line.source == segment.source && line.point1 == segment.point1 && line.point2 == segment.point2;
    };

    if (const auto it = std::find_if(m_lineSources.begin(), m_lineSources.end(), sameSegment); it != m_lineSources.end())
    {
        // A line already taken goes on a second click, whatever became of its endpoints since.
        m_lineSources.erase(it);
    }
    else
    {
        // A line is a segment drawn between the images of its two points: one endpoint without an image leaves
        // nothing to build, and showing the line in the preview would promise a copy that can never come. The
        // click is refused, and said — the same thing a point tool says when a point cannot be a pivot.
        if (!LineCanBeBuilt(segment))
        {
            emit ToolTip(tr("Take both points of this line first"));
            return;
        }

        m_lineSources.append(segment);
    }

    emit ToolTip(QString());
    OnSourceObjectsSet();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ChosenSegment(quint32 id, quint32 point1, quint32 point2)
{
    OOperationLine segment;
    segment.source = id;
    segment.point1 = point1;
    segment.point2 = point2;

    // Pen and colour are read back from the tool: the scene carries the two endpoints and nothing else. A pair the
    // tool does not declare keeps the defaults — better a line with the default pen than no line at all.
    if (auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::FindTool(id)); tool != nullptr)
    {
        const QVector<VDrawnSegment> drawn = tool->DrawnSegments();
        const auto it = std::find_if(drawn.cbegin(),
                                     drawn.cend(),
                                     [point1, point2](const VDrawnSegment &candidate) -> bool
                                     { return candidate.point1 == point1 && candidate.point2 == point2; });
        if (it != drawn.cend())
        {
            segment.typeLine = it->typeLine;
            segment.lineColor = it->lineColor;
        }
    }

    ToggleDrawnSegment(segment);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::PreviewLines() const -> QVector<QPair<quint32, quint32>>
{
    const QVector<OOperationLine> attached = OOperationLines::Collect(m_lineSources);

    QVector<QPair<quint32, quint32>> lines;
    lines.reserve(attached.size());
    for (const auto &line : attached)
    {
        lines.append(qMakePair(line.point1, line.point2));
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::HasImage(quint32 pointId) const -> bool
{
    if (pointId == NULL_ID)
    {
        return false;
    }

    return std::any_of(m_sourceObjects.cbegin(),
                       m_sourceObjects.cend(),
                       [pointId](const SourceItem &item) -> bool { return item.id == pointId; });
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::LineCanBeBuilt(const OOperationLine &line) const -> bool
{
    return HasImage(line.point1) && HasImage(line.point2);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::DropUnbuildableLines()
{
    const auto first = std::remove_if(m_lineSources.begin(),
                                      m_lineSources.end(),
                                      [this](const OOperationLine &line) -> bool { return !LineCanBeBuilt(line); });
    m_lineSources.erase(first, m_lineSources.end());
}

//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::CheckDependencyTreeComplete()
{
    // Nothing is gated on the dependency graph any more: the operated objects are picked on the canvas, one
    // click each, and the dialog no longer edits their names. The virtual stays — every operation tool calls
    // it, and DialogTool keeps it pure.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetNotes(const QString &notes) const
{
    Widgets().notes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetNotes() const -> QString
{
    return Widgets().notes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ClearSourceObjects()
{
    SetSourceObjects({});
    SetLineSources({});
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::GroupNameChanged()
{
    if (const auto *edit = qobject_cast<QLineEdit *>(sender()); edit)
    {
        if (edit->text().isEmpty())
        {
            flagGroupName = false;
            if (Widgets().labelGroupName != nullptr)
            {
                ChangeColor(Widgets().labelGroupName, errorColor);
            }
            Widgets().labelStatus->setText(tr("Invalid layer name"));
            CheckState();
            return;
        }

        flagGroupName = true;
        if (Widgets().labelGroupName != nullptr)
        {
            ChangeColor(Widgets().labelGroupName, OkColor(this));
        }
    }
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ToggleSourceObject(quint32 id)
{
    // One object, and only that one: the click toggles the object it landed on.
    const QVector<quint32> group{id};

    // Any member already present in the source list => the group is being removed, not added.
    bool present = false;
    for (const quint32 gid : group)
    {
        for (const SourceItem &sItem : m_sourceObjects)
        {
            if (sItem.id == gid)
            {
                present = true;
                break;
            }
        }
        if (present)
        {
            break;
        }
    }

    if (present)
    {
        for (const quint32 gid : group)
        {
            for (int i = 0; i < m_sourceObjects.size(); ++i)
            {
                if (m_sourceObjects.at(i).id == gid)
                {
                    m_sourceObjects.removeAt(i);
                    break;
                }
            }
        }

        // The lines this object carried have lost an endpoint: they cannot be built any more, and a preview that
        // kept them would show a copy that never comes.
        DropUnbuildableLines();
    }
    else
    {
        for (const quint32 gid : group)
        {
            bool alreadyThere = false;
            for (const SourceItem &sItem : m_sourceObjects)
            {
                if (sItem.id == gid)
                {
                    alreadyThere = true;
                    break;
                }
            }
            if (!alreadyThere)
            {
                m_sourceObjects.push_back(SourceItem{gid});
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::SourceObjectsChanged() const -> bool
{
    if (!m_sourceBaselineCaptured)
    {
        return false;
    }

    QVector<quint32> current;
    current.reserve(m_sourceObjects.size());
    for (const auto &item : m_sourceObjects)
    {
        current.append(item.id);
    }

    if (m_lineBaselineCaptured && !SameLines(m_lineSources, m_baselineLineSources))
    {
        // A line change needs the same full reparse as an object change: the transformed lines are
        // rebuilt from the operation element.
        return true;
    }

    return current != m_baselineSourceIds;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::CheckState()
{
    DialogTool::CheckState();

    if (bApply == nullptr)
    {
        return;
    }

    // Adding or removing operated objects forces a full reparse that recreates this tool and would
    // orphan the open dialog (breaking further Apply/Save). Disable Apply for such structural changes
    // while editing an existing tool - the user commits them with OK. Parameter-only edits (angle,
    // length, origin, rename) reparse lite and keep working with Apply.
    if (GetToolId() != NULL_ID && SourceObjectsChanged())
    {
        bApply->setEnabled(false);
        bApply->setToolTip(tr("Adding or removing objects can't be applied while the dialog stays open. "
                              "Use OK to confirm."));
    }
    else
    {
        bApply->setToolTip(QString());
    }
}
