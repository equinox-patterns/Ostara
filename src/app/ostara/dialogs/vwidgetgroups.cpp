/************************************************************************
 **
 **  @file   vwidgetgroups.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 4, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#include "vwidgetgroups.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/compatibility.h"
#include "vtheme.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/dialogs/tools/dialoggroup.h"
#include "../vtools/undocommands/undogroup.h"
#include "ui_vwidgetgroups.h"

#include <QCompleter>
#include <QDomElement>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QUndoStack>

namespace
{

constexpr int kIconSize = 20;

// Row layout: visibility + colour dot + name on the left, one action icon per column on the right.
constexpr int kVisible = 0;
constexpr int kColor = 1;
constexpr int kName = 2;
constexpr int kEdit = 3;
constexpr int kAdd = 4;
constexpr int kRemove = 5;
constexpr int kDelete = 6;

constexpr int kColumns = 7;

//---------------------------------------------------------------------------------------------------------------------
auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

//---------------------------------------------------------------------------------------------------------------------
// One themed icon. Normal and Selected both use the current theme variant: the picked row of the Layers table is
// tinted with the accent at low opacity, so in the light theme it stays a LIGHT background — the dark variant
// would be the wrong way round there. Hover keeps the dark variant, because the hovered row gets the full accent.
auto MakeCellIcon(const QString &baseName, bool dark) -> QIcon
{
    QIcon icon;
    icon.addFile(IconResPath(baseName, dark), QSize(), QIcon::Normal);
    icon.addFile(IconResPath(baseName, dark), QSize(), QIcon::Selected);
    icon.addFile(IconResPath(baseName, true), QSize(), QIcon::Active);
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
// The layer's colour, as a filled dot. The name is resolved against the current theme, so a black layer stays
// visible on the dark one — the same rule the objects themselves follow.
auto MakeColorDot(const QString &colorName, bool dark) -> QIcon
{
    constexpr int kDotDiameter = 12;

    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ColorNameToQColorForTheme(colorName, dark));
    const QPointF center(kIconSize / 2.0, kIconSize / 2.0);
    painter.drawEllipse(center, kDotDiameter / 2.0, kDotDiameter / 2.0);

    return QIcon(pixmap);
}

//---------------------------------------------------------------------------------------------------------------------
// Draws a centered cell icon. The base class would paint the icon left-aligned
// (SUBTILITES_CODE.md §16/§26); clearing the icon here lets us paint it centered.
class IconDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->icon = QIcon(); // Prevent the base paint() from drawing the icon left-aligned
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (icon.isNull())
        {
            return;
        }

        QIcon::Mode mode = QIcon::Normal;
        if (option.state & QStyle::State_Selected)
        {
            // Same variant as at rest: the row tint keeps a light background in the light theme.
            mode = QIcon::Selected;
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            // A hovered row gets the full accent, so there the dark variant is the readable one.
            mode = QIcon::Active;
        }

        const QSize iconSize(kIconSize, kIconSize);
        const QPoint center = option.rect.center();
        const QRect iconRect(center.x() - iconSize.width() / 2,
                             center.y() - iconSize.height() / 2, iconSize.width(), iconSize.height());
        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------
VWidgetGroups::VWidgetGroups(VContainer *pattern, VAbstractPattern *doc, QWidget *parent)
  : QWidget(parent),
    ui(new Ui::VWidgetGroups),
    m_doc(doc),
    m_pattern(pattern),
    m_addButton(new QPushButton(this))
{
    ui->setupUi(this);

    // cppcheck-suppress unknownMacro
    SCASSERT(doc != nullptr)
    SCASSERT(pattern != nullptr)

    BuildAddButton();

    QTableWidget *table = ui->tableWidget;
    // A name of its own, so the table says what it holds rather than `tableWidget`.
    table->setObjectName(QStringLiteral("layersTable"));
    // The .ui declares no column — create them before touching their widths or resize modes,
    // otherwise `setColumnWidth` / `setSectionResizeMode` silently do nothing and every column
    // falls back to the 100 px default.
    table->setColumnCount(kColumns);
    table->setShowGrid(false);
    // Rows are selected, but only ever by the widget: the selected row IS the active layer, so a click on an
    // action icon must not move the highlight away from it (OnCellClicked puts it back).
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Inline rename: double-click (or F2) on the name column only — every other cell is
    // non-editable (flags without Qt::ItemIsEditable).
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(kName, QHeaderView::Stretch);
    table->setColumnWidth(kVisible, 36);
    table->setColumnWidth(kColor, 36);
    table->setColumnWidth(kEdit, 44);
    table->setColumnWidth(kAdd, 44);
    table->setColumnWidth(kRemove, 44);
    table->setColumnWidth(kDelete, 44);
    // Compact fixed-height rows — the list never stretches rows vertically.
    table->verticalHeader()->setDefaultSectionSize(36);
    table->setIconSize(QSize(kIconSize, kIconSize));
    for (int col : {kVisible, kColor, kEdit, kAdd, kRemove, kDelete})
    {
        table->setItemDelegateForColumn(col, new IconDelegate(table));
    }

    FillTable(doc->GetGroups());
    UpdateActiveRowStyle();

    table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(table, &QTableWidget::cellClicked, this, &VWidgetGroups::OnCellClicked);
    connect(table, &QTableWidget::cellChanged, this, &VWidgetGroups::RenameGroup);
    connect(table, &QTableWidget::customContextMenuRequested, this, &VWidgetGroups::CtxMenu);
    connect(ui->lineEditTags, &QLineEdit::textChanged, this, &VWidgetGroups::UpdateGroups);

    // The dock stays open across theme toggles — the icons and the active-row tint must follow.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]()
    {
        UpdateAddButtonIcon();
        UpdateActiveRowStyle();
        UpdateGroups();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::UpdateActiveRowStyle()
{
    // The row of the active layer: the accent at low opacity — the tint of a checked QToolButton — instead of the
    // full accent, which reads as an alarm on a list that always shows one selected row.
    //
    // Applied on the table itself and not in the application stylesheet: a widget's own stylesheet beats the
    // application's whatever the selector specificity (doc-rendu, fiche 02, « Cascade »). An ID-scoped
    // `QTableWidget#layersTable::item:selected` rule in the global QSS was silently ignored.
    const QPalette palette = ui->tableWidget->palette();
    const QColor accent = palette.color(QPalette::Highlight); // filled by the QSS selection-background-color
    const QColor text = palette.color(QPalette::Text);
    // Half the palette's OverlaySelection token: on a list that always shows one row picked, even the overlay
    // reads as a band. Light gets 5 %, dark 10 % — the dark canvas needs a little more to be seen at all.
    const qreal alpha = VTheme::ColorSheme() == VColorSheme::Dark ? 0.10 : 0.05;

    const QString tint = QStringLiteral("rgba(%1, %2, %3, %4)")
                             .arg(accent.red())
                             .arg(accent.green())
                             .arg(accent.blue())
                             .arg(alpha);

    // BOTH properties, and on the widget's own stylesheet — measured, not guessed: with only `::item:selected`, or
    // only `selection-background-color`, the row keeps the full accent; together they give the tint.
    ui->tableWidget->setStyleSheet(QStringLiteral("QTableWidget { selection-background-color: %1; selection-color: %2; }"
                                                  "QTableWidget::item:selected { background-color: %1; color: %2; }")
                                       .arg(tint, text.name()));
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetGroups::~VWidgetGroups()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::BuildAddButton()
{
    m_addButton->setObjectName(QStringLiteral("addLayerButton"));
    m_addButton->setText(tr("Add a layer"));
    m_addButton->setIconSize(QSize(kIconSize, kIconSize));
    m_addButton->setFocusPolicy(Qt::NoFocus);
    m_addButton->setAutoDefault(false);
    connect(m_addButton, &QPushButton::clicked, this, &VWidgetGroups::ActionAddLayer);
    m_addButton->installEventFilter(this);
    UpdateAddButtonIcon();

    // Between the tag filter and the list — directly above it, like the "Add a size range" button.
    ui->verticalLayout->insertWidget(1, m_addButton, 0, Qt::AlignHCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::UpdateAddButtonIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    // Normal: current-theme variant. Hover: always the dark variant — the hovered button
    // background is an accent, so the lighter glyph keeps the contrast.
    m_addNormalIcon = QIcon(IconResPath(QStringLiteral("stack-plus-fill"), dark));
    m_addHoverIcon = QIcon(IconResPath(QStringLiteral("stack-plus-fill"), true));
    m_addButton->setIcon(m_addNormalIcon);
}

//---------------------------------------------------------------------------------------------------------------------
bool VWidgetGroups::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_addButton)
    {
        if (event->type() == QEvent::Enter)
        {
            m_addButton->setIcon(m_addHoverIcon);
        }
        else if (event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            m_addButton->setIcon(m_addNormalIcon);
        }
    }
    return QWidget::eventFilter(watched, event);
}

//----------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::SetGroupVisibility(vidtype id, bool visible) const
{
    auto *changeGroup = new ChangeGroupVisibility(m_doc, id, visible);
    connect(changeGroup, &ChangeGroupVisibility::UpdateGroup, this,
            [this](vidtype id, bool visible)
            {
                int const row = GroupRow(id);
                if (row == -1)
                {
                    return;
                }

                QTableWidgetItem *item = ui->tableWidget->item(row, kVisible);
                if (item)
                {
                    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
                    item->setIcon(MakeCellIcon(visible ? QStringLiteral("eye") : QStringLiteral("eye-closed"), dark));
                }
            });
    VAbstractApplication::VApp()->getUndoStack()->push(changeGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::SetMultipleGroupsVisibility(const QVector<vidtype> &groups, bool visible) const
{
    auto *changeGroups = new ChangeMultipleGroupsVisibility(m_doc, groups, visible);
    connect(changeGroups, &ChangeMultipleGroupsVisibility::UpdateMultipleGroups, this,
            [this](const QMap<vidtype, bool> &groups)
            {
                const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
                QMap<vidtype, bool>::const_iterator i = groups.constBegin();
                while (i != groups.constEnd())
                {
                    int const row = GroupRow(i.key());
                    if (row == -1)
                    {
                        ++i;
                        continue;
                    }

                    if (QTableWidgetItem *item = ui->tableWidget->item(row, kVisible); item)
                    {
                        item->setIcon(MakeCellIcon(i.value() ? QStringLiteral("eye")
                                                             : QStringLiteral("eye-closed"), dark));
                    }
                    ++i;
                }
            });
    VAbstractApplication::VApp()->getUndoStack()->push(changeGroups);
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::FilterGroups(const QMap<quint32, VGroupData> &groups) -> QMap<quint32, VGroupData>
{
    QMap<quint32, VGroupData> filtered;
    QSet<QString> const filterCategories =
        ConvertToSet<QString>(VAbstractPattern::FilterGroupTags(ui->lineEditTags->text()));

    if (filterCategories.isEmpty())
    {
        return groups;
    }

    auto i = groups.constBegin();
    while (i != groups.constEnd())
    {
        const VGroupData &data = i.value();
        if (QSet<QString> const groupCategories = ConvertToSet<QString>(data.tags);
            filterCategories.intersects(groupCategories))
        {
            filtered.insert(i.key(), data);
        }
        ++i;
    }

    return filtered;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::GroupRow(vidtype id) const -> int
{
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
    {
        QTableWidgetItem  const*item = ui->tableWidget->item(r, kVisible);
        SCASSERT(item != nullptr)

        if (id == item->data(Qt::UserRole).toUInt())
        {
            return r;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionAddLayer()
{
    // The dialog only collects a name and tags here: the new layer is empty, its objects are
    // added afterwards from the canvas. A throwaway container is enough to build it.
    QScopedPointer<VContainer> const fackeContainer(new VContainer(VAbstractApplication::VApp()->TrVars(),
                                                                   VAbstractValApplication::VApp()->patternUnitsP(),
                                                                   VContainer::UniqueNamespace()));
    QScopedPointer<DialogGroup> const dialog(new DialogGroup(fackeContainer.data(), m_doc, NULL_ID, this));
    dialog->SetGroupCategories(m_doc->GetGroupCategories());

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    const quint32 newId = m_pattern->getNextId();
    if (const QDomElement group = m_doc->CreateGroup(newId,
                                                     dialog->GetName(),
                                                     dialog->GetTags(),
                                                     {},
                                                     null_id,
                                                     dialog->GetColor(),
                                                     dialog->GetPenStyle());
        !group.isNull())
    {
        const quint32 previous = m_doc->GetActiveGroup();

        // The layer just created becomes the active one — it is where the next object will land. Both writes go
        // into a single undo step so one Ctrl+Z takes the layer away entirely.
        QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
        undoStack->beginMacro(tr("add a layer"));

        auto *addGroup = new AddGroup(group, m_doc);
        connect(addGroup, &AddGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        undoStack->push(addGroup);

        auto *change = new ChangeActiveGroup(m_doc, newId, previous);
        connect(change, &ChangeActiveGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        undoStack->push(change);

        undoStack->endMacro();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionEditLayer(quint32 id)
{
    QScopedPointer<VContainer> const fackeContainer(new VContainer(VAbstractApplication::VApp()->TrVars(),
                                                                   VAbstractValApplication::VApp()->patternUnitsP(),
                                                                   VContainer::UniqueNamespace()));
    QScopedPointer<DialogGroup> const dialog(new DialogGroup(fackeContainer.data(), m_doc, NULL_ID, this));
    dialog->SetName(m_doc->GetGroupName(id));
    dialog->SetTags(m_doc->GetGroupTags(id));
    dialog->SetColor(m_doc->GetGroupColor(id));
    dialog->SetPenStyle(m_doc->GetGroupPenStyle(id));
    dialog->SetGroupCategories(m_doc->GetGroupCategories());
    const int result = dialog->exec();

    if (result == QDialog::Accepted)
    {
        auto *changeGroupOptions = new ChangeGroupOptions(m_doc, id, dialog->GetName(), dialog->GetTags(),
                                                          dialog->GetColor(), dialog->GetPenStyle());
        connect(changeGroupOptions, &ChangeGroupOptions::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        // Through the document too: the canvas chrome (the rule's colour, the layer's name) is refreshed by
        // MainWindow, which listens to the document and not to this dock.
        connect(changeGroupOptions, &ChangeGroupOptions::UpdateGroups, m_doc,
                &VAbstractPattern::UpdateVisiblityGroups);
        VAbstractApplication::VApp()->getUndoStack()->push(changeGroupOptions);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionDeleteLayer(quint32 id)
{
    // A layer holding objects is not deletable — it must be emptied first. The icon stays clickable
    // so the refusal can be explained: a dimmed icon says "no" without telling why.
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    const auto group = groups.constFind(id);
    if (group != groups.constEnd() && not group.value().items.isEmpty())
    {
        const int count = static_cast<int>(group.value().items.size());
        QMessageBox::information(this, tr("Layer not empty"),
                                 tr("The layer \"%1\" still contains %n object(s). Remove them from the layer "
                                    "first, then delete it.",
                                    "", count)
                                     .arg(group.value().name));
        return;
    }

    auto *delGroup = new DelGroup(m_doc, id);
    connect(delGroup, &DelGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(delGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionHideAll()
{
    if (ui->tableWidget->rowCount() < 1)
    {
        return;
    }

    QVector<vidtype> groups;
    groups.reserve(ui->tableWidget->rowCount());
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
    {
        QTableWidgetItem  const*rowItem = ui->tableWidget->item(r, kVisible);
        quint32 const i = rowItem->data(Qt::UserRole).toUInt();
        if (m_doc->GetGroupVisibility(i))
        {
            groups.append(i);
        }
    }

    if (not groups.isEmpty())
    {
        SetMultipleGroupsVisibility(groups, false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionShowAll()
{
    if (ui->tableWidget->rowCount() < 1)
    {
        return;
    }

    QVector<vidtype> groups;
    groups.reserve(ui->tableWidget->rowCount());
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
    {
        QTableWidgetItem  const*rowItem = ui->tableWidget->item(r, kVisible);
        quint32 const i = rowItem->data(Qt::UserRole).toUInt();
        if (not m_doc->GetGroupVisibility(i))
        {
            groups.append(i);
        }
    }

    if (not groups.isEmpty())
    {
        SetMultipleGroupsVisibility(groups, true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    QTableWidgetItem  const*idItem = ui->tableWidget->item(row, kVisible);
    if (idItem == nullptr)
    {
        return;
    }
    const quint32 id = idItem->data(Qt::UserRole).toUInt();

    switch (column)
    {
        case kVisible:
            SetGroupVisibility(id, not m_doc->GetGroupVisibility(id));
            SelectActiveRow(); // the highlight belongs to the active layer, never to the last click
            break;
        case kName:
            // A single click on the name makes the layer active. A double click still renames it: the editor
            // opens on top of the same gesture, and RenameGroup only fires on the committed text.
            SetActiveLayer(id);
            break;
        case kEdit:
            ActionEditLayer(id);
            SelectActiveRow();
            break;
        case kDelete:
            ActionDeleteLayer(id);
            break;
        case kAdd:
            // Picking objects for this layer: the canvas does the clicking from now on, Enter confirms.
            StartObjectSelection(id, ObjectSelection::Adding);
            break;
        case kRemove:
            // The other direction, and the same gesture. What a removal may take out is narrowed to the objects
            // another layer also holds — see RemovableObjects.
            StartObjectSelection(id, ObjectSelection::Removing);
            break;
        default:
            SelectActiveRow();
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::StartObjectSelection(quint32 layerId, ObjectSelection mode)
{
    // Whatever was being picked is dropped first: the marks live on the canvas, and the new pass knows nothing
    // about them — a layer left half-picked would keep its orange for good.
    CancelObjectSelection();

    const QVector<QPair<quint32, quint32>> layerItems = LayerItems(layerId);

    // An empty layer has nothing to give up, and arming a removal over it would be a dead end: no click could ever
    // land on an object, and only Escape would get the user out.
    if (mode == ObjectSelection::Removing && layerItems.isEmpty())
    {
        emit ShowStatusTip(tr("This layer holds no object — there is nothing to remove."));
        SelectActiveRow();
        return;
    }

    m_selectionLayerId = layerId;
    m_objectSelection = mode;

    for (const auto &[objectId, toolId] : layerItems)
    {
        m_layerObjects.insert(objectId, toolId);
    }

    // The layer's content, magenta in both directions: it is what the layer holds, whether the user is adding to
    // it or emptying it. The clicks then turn the chosen ones orange — what Enter will act on.
    for (auto it = m_layerObjects.constBegin(); it != m_layerObjects.constEnd(); ++it)
    {
        emit ObjectInLayerChanged(it.key(), it.value(), true);
    }

    if (mode == ObjectSelection::Removing)
    {
        m_candidateObjects = RemovableObjects(layerItems);
    }

    emit ObjectSelectionStarted();
    UpdateSelectionTip();
    SelectActiveRow();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ToggleLayerObjects(const QVector<QPair<quint32, quint32>> &objects)
{
    if (not IsSelectingObjects() || objects.isEmpty())
    {
        return;
    }

    const bool removing = m_objectSelection == ObjectSelection::Removing;

    // What the layer can still take, or can still give up. Everything else stays out of the toggle, and a click
    // that reached nothing says why instead of silently doing nothing.
    QVector<QPair<quint32, quint32>> toggled;
    toggled.reserve(objects.size());
    for (const auto &object : objects)
    {
        const bool pickable = removing ? m_candidateObjects.value(object.first, NULL_ID) == object.second
                                       : m_layerObjects.value(object.first, NULL_ID) != object.second;
        if (pickable)
        {
            toggled.append(object);
        }
    }

    if (toggled.isEmpty())
    {
        if (removing)
        {
            ExplainRemovalRefusal(objects.constFirst());
        }
        else
        {
            ExplainAddRefusal();
        }
        return;
    }

    // A tool is picked or dropped as a whole: one click on a rectangle picks its points, a second takes them back.
    bool picked = false;
    for (const auto &[objectId, toolId] : toggled)
    {
        if (not m_selectedObjects.contains(objectId))
        {
            picked = true;
            break;
        }
    }

    for (const auto &[objectId, toolId] : toggled)
    {
        if (picked)
        {
            m_selectedObjects.insert(objectId, toolId);
        }
        else
        {
            m_selectedObjects.remove(objectId);
        }

        emit ObjectSelectedChanged(objectId, toolId, picked);
    }

    UpdateSelectionTip();
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::LayerItems(quint32 layerId) -> QVector<QPair<quint32, quint32>>
{
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    const auto layer = groups.constFind(layerId);
    return layer == groups.constEnd() ? QVector<QPair<quint32, quint32>>() : layer.value().items;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::RemovableObjects(const QVector<QPair<quint32, quint32>> &items) -> QMap<quint32, quint32>
{
    QMap<quint32, quint32> removable;

    // How many layers of this piece hold each object. Counted over the whole piece, not just the layer: the
    // question is precisely whether something ELSE holds it. A layer the tag filter hides counts like any other
    // — it is a real layer, and it really does hold the object.
    QMap<QPair<quint32, quint32>, int> holders;
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    for (auto group = groups.constBegin(); group != groups.constEnd(); ++group)
    {
        for (const auto &item : group.value().items)
        {
            ++holders[item];
        }
    }

    for (const auto &item : items)
    {
        if (holders.value(item) < 2)
        {
            continue;
        }

        // The tool has to be there. getTool() throws on an id it does not know, and the removal would then reach
        // it — the context menu writes items whose tool the document may no longer hold (an object an operation
        // keys by its own id, a stale entry). Those cannot be drawn, so they cannot be clicked either.
        if (VAbstractPattern::FindTool(item.second) == nullptr)
        {
            continue;
        }

        removable.insert(item.first, item.second);
    }

    return removable;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::UpdateSelectionTip()
{
    const int count = static_cast<int>(m_selectedObjects.size());
    if (count > 0)
    {
        emit ShowStatusTip(tr("%n object(s) selected — Enter to confirm, Escape cancels.", "", count));
        return;
    }

    if (m_objectSelection == ObjectSelection::Removing)
    {
        // The invariant, spelled out, and said again on every pass: it is the only reason a click can be refused.
        if (m_candidateObjects.isEmpty())
        {
            emit ShowStatusTip(tr("No object of this layer is shared with another layer — an object must always "
                                  "belong to at least one layer, so there is nothing to remove here."));
            return;
        }

        emit ShowStatusTip(tr("Select the objects to remove, then press Enter to confirm — Escape cancels. Magenta "
                              "is this layer's content; an object must always stay in one layer, so only what "
                              "another layer holds can go."));
        return;
    }

    emit ShowStatusTip(tr("Select the objects to add, then press Enter to confirm — Escape cancels. Magenta is this "
                          "layer's content, so those cannot be added again."));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ExplainRemovalRefusal(const QPair<quint32, quint32> &object)
{
    // Two different refusals, and saying the wrong one would send the user looking for a layer that is not there.
    const QMap<quint32, QString> holders = m_doc->GetGroupsContainingItem(object.second, object.first, true);
    if (holders.contains(m_selectionLayerId))
    {
        emit ShowStatusTip(tr("This object belongs only to this layer — an object must always belong to at least "
                              "one layer, so it cannot be removed."));
    }
    else
    {
        emit ShowStatusTip(tr("This object is not in this layer."));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ExplainAddRefusal()
{
    emit ShowStatusTip(tr("This object is already in this layer — a layer holds an object once."));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ValidateObjectSelection()
{
    if (not IsSelectingObjects())
    {
        return;
    }

    const quint32 layerId = m_selectionLayerId;
    const bool removing = m_objectSelection == ObjectSelection::Removing;

    if (not m_selectedObjects.isEmpty())
    {
        QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
        undoStack->beginMacro(removing ? tr("remove objects from a layer") : tr("add objects to a layer"));

        for (auto it = m_selectedObjects.constBegin(); it != m_selectedObjects.constEnd(); ++it)
        {
            // One command per object: the single writer that carries an undo with it, and the one the canvas
            // context menu already uses.
            if (removing)
            {
                if (const QDomElement item = m_doc->RemoveItemFromGroup(it.value(), it.key(), layerId);
                    not item.isNull())
                {
                    undoStack->push(new ::RemoveItemFromGroup(item, m_doc, layerId));
                }
            }
            else if (const QDomElement item = m_doc->AddItemToGroup(it.value(), it.key(), layerId);
                     not item.isNull())
            {
                undoStack->push(new ::AddItemToGroup(item, m_doc, layerId));
            }
        }

        undoStack->endMacro();
    }

    CancelObjectSelection();
    UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::CancelObjectSelection()
{
    if (not IsSelectingObjects())
    {
        return;
    }

    // The selection first — in a removal the selected objects are also the layer's content, so they have to go back
    // to magenta before the content itself is given back its own colour.
    for (auto it = m_selectedObjects.constBegin(); it != m_selectedObjects.constEnd(); ++it)
    {
        emit ObjectSelectedChanged(it.key(), it.value(), false);
    }

    for (auto it = m_layerObjects.constBegin(); it != m_layerObjects.constEnd(); ++it)
    {
        emit ObjectInLayerChanged(it.key(), it.value(), false);
    }

    m_selectedObjects.clear();
    m_layerObjects.clear();
    m_candidateObjects.clear();
    m_selectionLayerId = NULL_ID;
    m_objectSelection = ObjectSelection::Idle;
    emit ObjectSelectionFinished();
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::SetActiveLayer(quint32 id)
{
    const quint32 previous = m_doc->GetActiveGroup();
    if (previous == id)
    {
        return;
    }

    auto *change = new ChangeActiveGroup(m_doc, id, previous);
    connect(change, &ChangeActiveGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(change);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::SelectActiveRow()
{
    const int row = GroupRow(m_doc->GetActiveGroup());
    if (row == -1)
    {
        ui->tableWidget->clearSelection();
        return;
    }

    ui->tableWidget->selectRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::RenameGroup(int row, int column)
{
    if (column != kName)
    {
        return;
    }

    const quint32 id = ui->tableWidget->item(row, kVisible)->data(Qt::UserRole).toUInt();
    auto *renameGroup = new ::RenameGroup(m_doc, id, ui->tableWidget->item(row, column)->text());
    connect(renameGroup, &RenameGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    // The name is also what the canvas shows, and that chrome follows the document, not this dock.
    connect(renameGroup, &RenameGroup::UpdateGroups, m_doc, &VAbstractPattern::UpdateVisiblityGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(renameGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::CtxMenu(const QPoint &pos)
{
    QTableWidgetItem  const*item = ui->tableWidget->itemAt(pos);
    if (not item)
    {
        return;
    }

    const int row = item->row();
    item = ui->tableWidget->item(row, kVisible);
    const quint32 id = item->data(Qt::UserRole).toUInt();
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    auto MultipleChangeVisibilityTo = [this](bool visibility)
    {
        for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
        {
            QTableWidgetItem  const*rowItem = ui->tableWidget->item(r, kVisible);
            if (rowItem and visibility != m_doc->GetGroupVisibility(rowItem->data(Qt::UserRole).toUInt()))
            {
                return true;
            }
        }

        return false;
    };

    QScopedPointer<QMenu> const menu(new QMenu());
    QAction  const*triggerVisibilityMenu =
        m_doc->GetGroupVisibility(id)
            ? menu->addAction(MakeCellIcon(QStringLiteral("eye-closed"), dark), tr("Hide"))
            : menu->addAction(MakeCellIcon(QStringLiteral("eye"), dark), tr("Show"));

    QAction  const*actionEdit = menu->addAction(MakeCellIcon(QStringLiteral("gear-six"), dark), tr("Edit"));
    QAction  const*actionDelete = menu->addAction(MakeCellIcon(QStringLiteral("trash"), dark), tr("Delete"));
    menu->addSeparator();
    QAction *actionHideAll = menu->addAction(tr("Hide All"));
    actionHideAll->setEnabled(MultipleChangeVisibilityTo(false));
    QAction *actionShowAll = menu->addAction(tr("Show All"));
    actionShowAll->setEnabled(MultipleChangeVisibilityTo(true));

    QAction  const*selectedAction = menu->exec(ui->tableWidget->viewport()->mapToGlobal(pos));

    if (selectedAction == triggerVisibilityMenu)
    {
        SetGroupVisibility(id, not m_doc->GetGroupVisibility(id));
    }
    else if (selectedAction == actionEdit)
    {
        ActionEditLayer(id);
    }
    else if (selectedAction == actionDelete)
    {
        ActionDeleteLayer(id);
    }
    else if (selectedAction == actionHideAll)
    { // all groups in "group" make unvisible
        ActionHideAll();
    }
    else if (selectedAction == actionShowAll)
    { // all groups in "group" make visible
        ActionShowAll();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::UpdateGroups()
{
    FillTable(m_doc->GetGroups());
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::RetranslateUI()
{
    ui->retranslateUi(this);

    // ⚠️ Le bouton et l'infobulle de catégories sont écrits en code, et `ui->retranslateUi` ne les
    // voit pas. `UpdateGroups` réécrit l'infobulle sans reconstruire le widget : aucune connexion
    // n'est rejouée, contrairement à un appel du constructeur.
    m_addButton->setText(tr("Add a layer"));
    UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::FillTable(QMap<quint32, VGroupData> groups)
{
    ui->lineEditTags->SetCompletion(m_doc->GetGroupCategories());

    groups = FilterGroups(groups);

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QSignalBlocker blocker(ui->tableWidget);
    // `clearContents`, not `clear`: keep the header configuration (column widths and resize
    // modes) set up by the constructor.
    ui->tableWidget->clearContents();

    ui->tableWidget->setColumnCount(kColumns);
    ui->tableWidget->setRowCount(static_cast<int>(groups.size()));
    qint32 currentRow = -1;
    auto i = groups.constBegin();
    while (i != groups.constEnd())
    {
        ++currentRow;
        const VGroupData data = i.value();

        auto *visibleItem = new QTableWidgetItem(
            MakeCellIcon(data.visible ? QStringLiteral("eye") : QStringLiteral("eye-closed"), dark), QString());
        visibleItem->setData(Qt::UserRole, i.key());
        // set the item non-editable (view only)
        visibleItem->setFlags(visibleItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(currentRow, kVisible, visibleItem);

        // The layer's colour, so a glance at the list tells which pen its objects are drawn with.
        auto *colorItem = new QTableWidgetItem(MakeColorDot(data.color, dark), QString());
        colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(currentRow, kColor, colorItem);

        auto *nameItem = new QTableWidgetItem(data.name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        if (not data.tags.isEmpty())
        {
            nameItem->setToolTip(tr("Categories: %1.").arg(data.tags.join(QStringLiteral(", "))));
        }

        if (data.items.isEmpty())
        {
            QFont font = nameItem->font();
            font.setStrikeOut(true);
            nameItem->setFont(font);
        }

        ui->tableWidget->setItem(currentRow, kName, nameItem);

        auto MakeActionItem = [this, currentRow](int column, const QIcon &icon, const QString &tooltip)
        {
            auto *actionItem = new QTableWidgetItem(icon, QString());
            actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
            if (not tooltip.isEmpty())
            {
                actionItem->setToolTip(tooltip);
            }
            ui->tableWidget->setItem(currentRow, column, actionItem);
        };

        MakeActionItem(kEdit, MakeCellIcon(QStringLiteral("gear-six"), dark), QString());
        MakeActionItem(kAdd, MakeCellIcon(QStringLiteral("plus"), dark), QString());
        MakeActionItem(kRemove, MakeCellIcon(QStringLiteral("minus"), dark), QString());
        // The trash stays fully visible and clickable even on a layer holding objects: the refusal
        // is explained by a dialog (ActionDeleteLayer), not by a dimmed icon the reader cannot read.
        MakeActionItem(kDelete, MakeCellIcon(QStringLiteral("trash"), dark), QString());

        ++i;
    }
    ui->tableWidget->sortItems(kName, Qt::AscendingOrder);
    SelectActiveRow();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUI();
    }
    QWidget::changeEvent(event);
}
