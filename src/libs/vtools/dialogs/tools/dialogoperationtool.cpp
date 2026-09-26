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

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSharedPointer>
#include <QStringList>

#include "../support/dialogbulkrename.h"
#include "vlineedit.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// The pattern object behind a source id, or nullptr when the id is not a pattern object at all: an operation can
// carry a line, which is a Line tool and not an object of the container. Reading a source must never throw.
auto SourceObject(const VContainer &data, quint32 id) -> QSharedPointer<VGObject>
{
    try
    {
        return data.GetGObject(id);
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
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
    return Widgets().visibilityGroupLine->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetVisibilityGroupName(const QString &name) const
{
    if (name.isEmpty())
    {
        // Nothing to put in the field: it keeps the default name its own .ui gives it, which is the name of the
        // operation and is translated for it. Writing one here would label every operation "Rotation".
        return;
    }

    Widgets().visibilityGroupLine->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::HasLinkedVisibilityGroup() const -> bool
{
    return Widgets().visibilityGroup->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetHasLinkedVisibilityGroup(bool linked) const
{
    Widgets().visibilityGroup->setChecked(linked);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetVisibilityGroupTags(const QStringList &tags) const
{
    Widgets().groupTags->setText(tags.join(", "));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetVisibilityGroupTags() const -> QStringList
{
    return Widgets().groupTags->text().split(',');
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetGroupCategories(const QStringList &categories)
{
    m_groupTags = categories;
    Widgets().groupTags->SetCompletion(m_groupTags);
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
        // First call populates the dialog from the tool; remember the committed object set (by recordId)
        // so later edits (add/remove) can be detected. Subsequent calls (add/remove/rename) must not move
        // the baseline.
        m_baselineSourceRecords.clear();
        m_baselineSourceRecords.reserve(value.size());
        for (const auto &item : value)
        {
            m_baselineSourceRecords.insert(item.recordId);
        }
        m_sourceBaselineCaptured = true;
    }

    FillSourceList();
    OnSourceObjectsSet();
    CheckState(); // adding/removing objects toggles the Apply button (see CheckState)
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetDestinationObjects(const QVector<DestinationItem> &value)
{
    m_destination = value;
    CurrentObjectChanged(Widgets().sourceList->currentRow());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::GetLineSources() const -> QVector<quint32>
{
    return m_lineSources;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::SetLineSources(const QVector<quint32> &value)
{
    m_lineSources = value;

    if (!m_lineBaselineCaptured)
    {
        m_baselineLineSources.clear();
        m_baselineLineSources.reserve(value.size());
        for (auto id : value)
        {
            m_baselineLineSources.insert(id);
        }
        m_lineBaselineCaptured = true;
    }

    FillSourceList();
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

    if (m_lineSources.contains(id))
    {
        m_lineSources.removeAll(id);
    }
    else
    {
        m_lineSources.append(id);
    }

    FillSourceList();
    OnSourceObjectsSet();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::PreviewLines() const -> QVector<QPair<quint32, quint32>>
{
    const QVector<OOperationLine> attached = OOperationLines::Collect(&data, m_sourceObjects, m_lineSources);

    QVector<QPair<quint32, quint32>> lines;
    lines.reserve(attached.size());
    for (const auto &line : attached)
    {
        lines.append(qMakePair(line.point1, line.point2));
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::IsLineRow(int row) const -> bool
{
    const QListWidgetItem *item = Widgets().sourceList->item(row);
    return item != nullptr && item->data(Qt::UserRole + 1).isValid();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::LineDisplayName(quint32 id) -> QString
{
    // Displaying a line must never fail, even if the line or one of its points is gone: fall back to a
    // generic name rather than letting an exception escape.
    try
    {
        if (auto *line = qobject_cast<VToolLine *>(VAbstractPattern::FindTool(id)); line != nullptr)
        {
            const QString first = line->FirstPointName();
            const QString second = line->SecondPointName();
            if (!first.isEmpty() && !second.isEmpty())
            {
                return tr("Line_%1_%2").arg(first, second);
            }
        }
    }
    catch (const VException &e)
    {
        Q_UNUSED(e)
    }

    return tr("Line");
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::CheckDependencyTreeComplete()
{
    m_dependencyReady = m_doc->IsPatternGraphComplete();
    Widgets().name->setEnabled(m_dependencyReady);
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
            ChangeColor(Widgets().labelGroupName, errorColor);
            Widgets().labelStatus->setText(tr("Invalid layer name"));
            CheckState();
            return;
        }

        flagGroupName = true;
        ChangeColor(Widgets().labelGroupName, OkColor(this));
    }
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ShowSourceDetails(int row)
{
    if (IsLineRow(row))
    {
        // A line carries no label, pen style nor colour of its own in this dialog: it is described by
        // the two points it links.
        Widgets().name->setDisabled(true);
        Widgets().penStyle->setDisabled(true);
        Widgets().color->setDisabled(true);
        return;
    }

    auto *combo = Widgets().color;

    Widgets().name->setDisabled(true);
    Widgets().penStyle->setDisabled(true);
    combo->setDisabled(true);

    if (Widgets().sourceList->count() == 0)
    {
        return;
    }

    const auto *item = Widgets().sourceList->item(row);
    if (item == nullptr)
    {
        return;
    }

    const auto sourceItem = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
    const QSharedPointer<VGObject> sourceObject = SourceObject(data, sourceItem.id);
    if (sourceObject != nullptr && sourceObject->getType() == GOType::Point)
    {
        const QSignalBlocker blockerPenStyle(Widgets().penStyle);
        const QSignalBlocker blockerColor(combo);

        Widgets().penStyle->setCurrentIndex(-1);
        combo->setCurrentIndex(-1);
    }
    else
    {
        auto SetValue = [](QComboBox *box, const QString &value, const QString &def) -> void
        {
            const QSignalBlocker blocker(box);

            if (const int index = box->findData(value); index != -1)
            {
                box->setCurrentIndex(index);
            }
            else
            {
                box->setCurrentIndex(box->findData(def));
            }
        };

        SetValue(Widgets().penStyle, sourceItem.penStyle, TypeLineDefault);

        if (sourceItem.penStyle.isEmpty() || sourceItem.penStyle == TypeLineDefault)
        {
            int const index = Widgets().penStyle->currentIndex();
            Widgets().penStyle->setItemText(index, '<' + tr("Default") + '>');
        }

        QSignalBlocker blockerColor(combo);
        const QColor sourceColor(sourceItem.color);
        SetColorComboCurrentName(combo, sourceColor.isValid() ? sourceColor.name() : ColorDefault);
        blockerColor.unblock();

        if (sourceItem.color.isEmpty() || sourceItem.color == ColorDefault)
        {
            if (const QSharedPointer<VAbstractCurve> curve = qSharedPointerDynamicCast<VAbstractCurve>(sourceObject);
                curve != nullptr)
            {
                SetColorComboCurrentName(combo, curve->GetColor());
            }
        }

        Widgets().penStyle->setEnabled(true);
        combo->setEnabled(true);
    }

    const QSignalBlocker blockerName(Widgets().name);
    Widgets().name->setText(sourceItem.name);
    Widgets().name->setEnabled(m_dependencyReady);

    const QVector<SourceItem> sourceObjects = SaveSourceObjects();
    const QSet<QString> freeNames = FindFreeNames(m_sourceObjects, sourceObjects);
    const bool nameValid = IsValidSourceName(sourceItem.name, sourceItem.id, m_sourceObjects, &data, freeNames);
    ChangeColor(Widgets().labelName, nameValid ? OkColor(this) : errorColor);
    flagName = nameValid;
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::CurrentObjectChanged(int row) const
{
    if (IsLineRow(row))
    {
        // A line is never committed on its own: dropping it from the list is always safe.
        Widgets().removeObject->setEnabled(true);
        return;
    }

    if (Widgets().sourceList->count() < 2)
    {
        Widgets().removeObject->setDisabled(true);
        return;
    }

    const auto *item = Widgets().sourceList->item(row);
    if (item == nullptr)
    {
        Widgets().removeObject->setDisabled(true);
        return;
    }

    const auto target = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
    const QUuid recordId = target.recordId;

    const auto it = std::find_if(m_destination.cbegin(),
                                 m_destination.cend(),
                                 [recordId](const DestinationItem &item) -> bool { return item.recordId == recordId; });

    // A source object that is not yet present in the destination list has not been committed to the
    // pattern, so no other tool can depend on it - it is always safe to remove. Only check safety for
    // objects that already exist in the pattern.
    if (it != m_destination.cend() && !IsSafeToRemoveGroupObject(it->id, m_doc))
    {
        Widgets().removeObject->setDisabled(true);
        return;
    }

    Widgets().removeObject->setEnabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::NameChanged(const QString &text)
{
    if (IsLineRow(Widgets().sourceList->currentRow()))
    {
        return;
    }

    if (Widgets().sourceList->count() == 0)
    {
        return;
    }

    if (auto *item = Widgets().sourceList->currentItem())
    {
        auto sourceItem = qvariant_cast<SourceItem>(item->data(Qt::UserRole));

        const QVector<SourceItem> objects = SaveSourceObjects();
        const QSet<QString> freeNames = FindFreeNames(m_sourceObjects, objects);
        const bool valid = IsValidSourceName(text, sourceItem.id, objects, &data, freeNames);

        if (valid)
        {
            sourceItem.name = text;
            item->setData(Qt::UserRole, QVariant::fromValue(sourceItem));
        }

        const QSharedPointer<VGObject> obj = SourceObject(data, sourceItem.id);
        ChangeColor(Widgets().labelName, valid ? OkColor(this) : errorColor);

        if (!valid)
        {
            Widgets().labelStatus->setText(obj != nullptr && obj->getType() == GOType::Point ? tr("Invalid label")
                                                                                            : tr("Invalid name"));
        }

        flagName = valid;
        CheckState();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::PenStyleChanged() const
{
    if (IsLineRow(Widgets().sourceList->currentRow()))
    {
        return;
    }

    if (Widgets().sourceList->count() == 0)
    {
        return;
    }

    if (auto *item = Widgets().sourceList->currentItem())
    {
        auto sourceItem = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
        sourceItem.penStyle = GetComboBoxCurrentData(Widgets().penStyle, TypeLineDefault);
        item->setData(Qt::UserRole, QVariant::fromValue(sourceItem));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ObjectTypeChanged(int index) const
{
    Widgets().addObject->setDisabled(true);

    const QSignalBlocker blocker(Widgets().newObject);
    Widgets().newObject->clear();

    if (index == -1)
    {
        return;
    }

    auto const type = Widgets().objectType->itemData(index).value<GOType>();

    switch (type)
    {
        case GOType::Point:
            FillComboBoxPoints(Widgets().newObject);
            break;
        case GOType::Arc:
            FillComboBoxArcs(Widgets().newObject);
            break;
        case GOType::EllipticalArc:
            FillComboBoxEllipticalArcs(Widgets().newObject);
            break;
        case GOType::Spline:
        case GOType::CubicBezier:
            FillComboBoxSplines(Widgets().newObject);
            break;
        case GOType::SplinePath:
        case GOType::CubicBezierPath:
            FillComboBoxSplinesPath(Widgets().newObject);
            break;
        case GOType::PlaceLabel:
        case GOType::Unknown:
        default:
            return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::NewObjectChanged() const
{
    quint32 const id = getCurrentObjectId(Widgets().newObject);
    if (id == NULL_ID)
    {
        return;
    }

    const QVector<SourceItem> sourceObjects = SaveSourceObjects();
    const auto it = std::find_if(sourceObjects.cbegin(),
                                 sourceObjects.cend(),
                                 [id](const SourceItem &item) -> bool { return item.id == id; });
    Widgets().addObject->setDisabled(it != sourceObjects.cend());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::AddNewObject()
{
    quint32 const id = getCurrentObjectId(Widgets().newObject);
    if (id == NULL_ID)
    {
        return;
    }

    QVector<SourceItem> sourceObjects = SaveSourceObjects();
    QSet<QString> occupiedNames;
    for (const auto &sourceItem : std::as_const(sourceObjects))
    {
        if (!sourceItem.name.isEmpty())
        {
            occupiedNames.insert(sourceItem.name);
        }
    }

    sourceObjects.append({.id = id, .name = GetDefSourceName(id, &data, "m"_L1, occupiedNames)});
    SetSourceObjects(sourceObjects);
    Widgets().addObject->setDisabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ColorChanged() const
{
    if (IsLineRow(Widgets().sourceList->currentRow()))
    {
        return;
    }

    if (Widgets().sourceList->count() == 0)
    {
        return;
    }

    if (auto *item = Widgets().sourceList->currentItem())
    {
        auto sourceItem = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
        const QString colorName = GetColorComboCurrentName(Widgets().color);
        const QColor color(colorName);
        sourceItem.color = color.isValid() ? color.name() : ColorDefault;
        item->setData(Qt::UserRole, QVariant::fromValue(sourceItem));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::BulkRename()
{
    if (DialogBulkRename dlg(SaveSourceObjects(), data, this); dlg.exec() == QDialog::Accepted && dlg.HasChanges())
    {
        SetSourceObjects(dlg.RenamedItems());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::RemoveObject()
{
    if (IsLineRow(Widgets().sourceList->currentRow()))
    {
        const QListWidgetItem *item = Widgets().sourceList->currentItem();
        m_lineSources.removeAll(item->data(Qt::UserRole + 1).toUInt());
        FillSourceList();
        OnSourceObjectsSet();
        CheckState();
        Widgets().removeObject->setDisabled(true);
        return;
    }

    if (Widgets().sourceList->count() < 2)
    {
        Widgets().removeObject->setDisabled(true);
        return;
    }

    const QListWidgetItem *item = Widgets().sourceList->currentItem();
    if (item == nullptr)
    {
        return;
    }

    const auto target = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
    const QUuid recordId = target.recordId;

    const auto it = std::find_if(m_destination.cbegin(),
                                 m_destination.cend(),
                                 [recordId](const DestinationItem &item) -> bool { return item.recordId == recordId; });

    // A source object that is not yet present in the destination list has not been committed to the
    // pattern, so no other tool can depend on it - it is always safe to remove. Only check safety for
    // objects that already exist in the pattern.
    if (it != m_destination.cend() && !IsSafeToRemoveGroupObject(it->id, m_doc))
    {
        Widgets().removeObject->setDisabled(true);
        return;
    }

    const int currentRow = Widgets().sourceList->row(item);
    delete Widgets().sourceList->takeItem(currentRow);

    SetSourceObjects(SaveSourceObjects());

    Widgets().removeObject->setDisabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::FillSourceList()
{
    QSignalBlocker blocker(Widgets().sourceList);

    Widgets().sourceList->clear();

    int row = -1;

    for (const auto &sourceItem : std::as_const(m_sourceObjects))
    {
        // A source the container does not know — a line the operation carries, an object that is gone — must not
        // blow the dialog up: the row stays, named after the line it is.
        const QSharedPointer<VGObject> obj = SourceObject(data, sourceItem.id);
        const QString name = obj != nullptr ? obj->ObjectName() : LineDisplayName(sourceItem.id);
        auto *item = new QListWidgetItem(name);
        item->setToolTip(name);
        item->setData(Qt::UserRole, QVariant::fromValue(sourceItem));
        Widgets().sourceList->insertItem(++row, item);
    }

    // Lines come last: they are carried by the operation beside its objects, and the row flags them with
    // the second data role so every handler can tell them apart.
    for (auto id : std::as_const(m_lineSources))
    {
        auto *item = new QListWidgetItem(LineDisplayName(id));
        item->setToolTip(item->text());
        item->setData(Qt::UserRole + 1, id);
        Widgets().sourceList->insertItem(++row, item);
    }

    blocker.unblock();

    if (Widgets().sourceList->count() > 0)
    {
        Widgets().sourceList->setCurrentRow(0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogOperationTool::SaveSourceObjects() const -> QVector<SourceItem>
{
    QVector<SourceItem> objects;
    objects.reserve(Widgets().sourceList->count());

    for (int i = 0; i < Widgets().sourceList->count(); ++i)
    {
        if (const QListWidgetItem *item = Widgets().sourceList->item(i))
        {
            if (item->data(Qt::UserRole + 1).isValid())
            {
                continue; // A line row: not an operated object
            }

            const auto sourceItem = qvariant_cast<SourceItem>(item->data(Qt::UserRole));
            objects.append(sourceItem);
        }
    }

    return objects;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogOperationTool::ToggleSourceObject(quint32 id)
{
    // When isolating a single object, do not expand to the whole attachment.
    // Otherwise, if the clicked object belongs to an attachment, toggle the whole group.
    const auto attachmentItems = m_doc->IsIsolated() ? QVector<QPair<quint32, quint32>>{}
                                                     : m_doc->GetAttachmentItemsForObject(id);

    QVector<quint32> group;
    if (attachmentItems.isEmpty())
    {
        group.append(id);
    }
    else
    {
        group.reserve(attachmentItems.size());
        for (const auto &item : attachmentItems)
        {
            group.append(item.first); // object id
        }
    }

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

    QSet<QUuid> current;
    current.reserve(m_sourceObjects.size());
    for (const auto &item : m_sourceObjects)
    {
        current.insert(item.recordId);
    }

    if (m_lineBaselineCaptured)
    {
        // A line change needs the same full reparse as an object change: the transformed lines are
        // rebuilt from the operation element.
        QSet<quint32> currentLines;
        currentLines.reserve(m_lineSources.size());
        for (auto id : std::as_const(m_lineSources))
        {
            currentLines.insert(id);
        }

        if (currentLines != m_baselineLineSources)
        {
            return true;
        }
    }

    return current != m_baselineSourceRecords;
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
