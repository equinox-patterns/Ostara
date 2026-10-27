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
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 19/10/2026.
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
#include "../vtools/dialogs/widgets/olayergroupchooser.h"
#include "../vtools/undocommands/olayergroupcommands.h"
#include "../vtools/undocommands/undogroup.h"
#include "ochooselayerdialog.h"
#include "olayergroupdialog.h"
#include "otransferlayersdialog.h"
#include "ui_vwidgetgroups.h"

#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QDomElement>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QUndoStack>

#include <algorithm>

namespace
{

constexpr int kIconSize = 20;

// An action column holds one 20 px icon and nothing else, and the item delegate centres it in the whole
// cell — so 28 px is the icon plus 4 px of air on either side. Every pixel spared here goes to the name
// column, which is the one thing in this table that has to be read rather than aimed at. The header is
// hidden (.ui), so no column title ever asks for more room than the icon.
constexpr int kIconColumnWidth = 28;

// Row layout: visibility + colour dot + name on the left, one action icon per column on the right.
constexpr int kVisible = 0;
constexpr int kColor = 1;
constexpr int kName = 2;
constexpr int kEdit = 3;
constexpr int kAdd = 4;
// Between the two directions: adding and removing already say where an object stands, moving it is the
// third thing one can do with it, so it reads between them.
constexpr int kTransfer = 5;
constexpr int kRemove = 6;
constexpr int kDelete = 7;

constexpr int kColumns = 8;

// The layer (or the group of layers) a row stands for rides in Qt::UserRole; what KIND of row it is rides in the
// role after it. A group row carries a group id in the very column a layer row carries a layer id, and both ids
// come from the same document counter — nothing but this marker can tell the two rows apart.
constexpr int kIdRole = Qt::UserRole;
constexpr int kKindRole = Qt::UserRole + 1;

//---------------------------------------------------------------------------------------------------------------------
// One action cell: a themed icon and nothing else. The text stays empty on purpose — the column has no room for a
// word, and the tooltip is what explains the icon.
void SetActionCell(QTableWidget *table, int row, int column, const QIcon &icon, const QString &tooltip = QString())
{
    auto *item = new QTableWidgetItem(icon, QString());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (not tooltip.isEmpty())
    {
        item->setToolTip(tooltip);
    }
    table->setItem(row, column, item);
}

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

//---------------------------------------------------------------------------------------------------------------------
// The name column wears the group's caret and is painted by the base delegate, which only ever asks an icon for
// Normal or Selected — never for the hovered state the action columns get from IconDelegate's Active mode. The
// caret of a hovered group would therefore stay the theme's variant on the accent-filled row. The icon's Active
// pixmap IS the dark variant (MakeCellIcon), so it is handed to the style as the only one.
class LayerNameDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        // A chosen row is only tinted (UpdateActiveRowStyle) and keeps the theme's variant; a hovered one takes the
        // full accent, where the dark set is the readable one.
        const bool hovered = (opt.state & QStyle::State_MouseOver) != 0
                             && (opt.state & QStyle::State_Selected) == 0;
        if (hovered && not opt.icon.isNull())
        {
            const QSize size = opt.decorationSize.isValid() ? opt.decorationSize : QSize(kIconSize, kIconSize);
            const QPixmap pixmap = opt.icon.pixmap(size, QIcon::Active);
            if (not pixmap.isNull())
            {
                opt.icon = QIcon(pixmap);
            }
        }

        const QWidget *widget = opt.widget;
        QStyle *style = widget != nullptr ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
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

    BuildAddButtons();
    BuildShowActiveLayerCheckBox();

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
    // The seven icon columns get the same width, the name column stretches over all that is left:
    // the dock is narrow, and widening it is not what is wanted — the icons give back what they take.
    for (int col : {kVisible, kColor, kEdit, kAdd, kTransfer, kRemove, kDelete})
    {
        table->setColumnWidth(col, kIconColumnWidth);
        table->setItemDelegateForColumn(col, new IconDelegate(table));
    }
    // The caret rides the name, so that column needs the same hover variant as the action icons.
    table->setItemDelegateForColumn(kName, new LayerNameDelegate(table));
    // Compact fixed-height rows — the list never stretches rows vertically.
    table->verticalHeader()->setDefaultSectionSize(36);
    table->setIconSize(QSize(kIconSize, kIconSize));

    FillTable(doc->GetGroups());
    UpdateActiveRowStyle();

    table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(table, &QTableWidget::cellClicked, this, &VWidgetGroups::OnCellClicked);
    connect(table, &QTableWidget::cellChanged, this, &VWidgetGroups::RenameGroup);
    connect(table, &QTableWidget::customContextMenuRequested, this, &VWidgetGroups::CtxMenu);
    connect(ui->lineEditLayerFilter, &QLineEdit::textChanged, this, &VWidgetGroups::UpdateGroups);

    // The dock stays open across theme toggles — the icons and the active-row tint must follow.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]()
    {
        UpdateAddButtonIcon();
        UpdateAddGroupButtonIcon();
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
void VWidgetGroups::BuildAddButtons()
{
    m_addButton->setObjectName(QStringLiteral("addLayerButton"));
    m_addButton->setText(tr("Add a layer"));
    m_addButton->setIconSize(QSize(kIconSize, kIconSize));
    m_addButton->setFocusPolicy(Qt::NoFocus);
    m_addButton->setAutoDefault(false);
    connect(m_addButton, &QPushButton::clicked, this, &VWidgetGroups::ActionAddLayer);
    m_addButton->installEventFilter(this);
    UpdateAddButtonIcon();

    m_addGroupButton = new QPushButton(this);
    m_addGroupButton->setObjectName(QStringLiteral("addLayerGroupButton"));
    m_addGroupButton->setText(tr("Add a group"));
    m_addGroupButton->setIconSize(QSize(kIconSize, kIconSize));
    m_addGroupButton->setFocusPolicy(Qt::NoFocus);
    m_addGroupButton->setAutoDefault(false);
    connect(m_addGroupButton, &QPushButton::clicked, this, &VWidgetGroups::ActionAddLayerGroup);
    m_addGroupButton->installEventFilter(this);
    UpdateAddGroupButtonIcon();

    // Between the filter and the list, the two of them side by side — and a LAYOUT, never a QWidget wrapper: the
    // viewport of a dock carries `.QWidget { background: $DockTitle }`, so a wrapper would draw a band behind the
    // buttons (subtilité §13).
    auto *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    buttons->addWidget(m_addButton);
    buttons->addWidget(m_addGroupButton);
    buttons->addStretch(1);
    ui->verticalLayout->insertLayout(1, buttons);
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
void VWidgetGroups::UpdateAddGroupButtonIcon()
{
    if (m_addGroupButton == nullptr)
    {
        return;
    }

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_addGroupNormalIcon = QIcon(IconResPath(QStringLiteral("stack-fill"), dark));
    m_addGroupHoverIcon = QIcon(IconResPath(QStringLiteral("stack-fill"), true));
    m_addGroupButton->setIcon(m_addGroupNormalIcon);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::BuildShowActiveLayerCheckBox()
{
    // The switch belongs to the dock that names the active layer: it is the same list the user reads, and the
    // canvas chrome is that list's answer drawn over the drawing. Off, it exists to read the drawing alone.
    m_showActiveLayer = new QCheckBox(tr("Show the active layer on the canvas"), this);
    m_showActiveLayer->setObjectName(QStringLiteral("showActiveLayerOnCanvas"));
    // Set BEFORE connecting, so `toggled` only ever carries a real user change — MainWindow persists on it.
    m_showActiveLayer->setChecked(VAbstractApplication::VApp()->Settings()->GetShowActiveLayerOnCanvas());
    connect(m_showActiveLayer, &QCheckBox::toggled, this, &VWidgetGroups::ShowActiveLayerOnCanvasChanged);

    // Straight into the dock's layout, and never into a QWidget wrapper of its own: the viewport of a dock
    // carries `.QWidget { background: $DockTitle }` (DockWrapper, subtilité §13), which only matches bare
    // QWidget instances — a QCheckBox is left transparent, a wrapper would get a band behind it.
    ui->verticalLayout->insertWidget(2, m_showActiveLayer, 0, Qt::AlignHCenter);
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
    else if (watched == m_addGroupButton)
    {
        if (event->type() == QEvent::Enter)
        {
            m_addGroupButton->setIcon(m_addGroupHoverIcon);
        }
        else if (event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            m_addGroupButton->setIcon(m_addGroupNormalIcon);
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

                // The group the layer belongs to, if any, reads its layers: hiding the last visible one of a group
                // closes its eye too, and that line has to follow.
                RefreshGroupEyes();
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

                // A group eye follows its layers: hiding the last visible one of a group closes it too.
                RefreshGroupEyes();
            });
    VAbstractApplication::VApp()->getUndoStack()->push(changeGroups);
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::KindOfRow(int row) const -> RowKind
{
    const QTableWidgetItem *item = ui->tableWidget->item(row, kVisible);
    if (item == nullptr)
    {
        return RowKind::Layer;
    }

    // A row built before the groups existed carries no marker, and every such row is a layer.
    return item->data(kKindRole).toUInt() == static_cast<quint32>(RowKind::LayerGroup) ? RowKind::LayerGroup
                                                                                       : RowKind::Layer;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::GroupRow(vidtype id) const -> int
{
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
    {
        if (KindOfRow(r) != RowKind::Layer)
        {
            continue;
        }

        const QTableWidgetItem *item = ui->tableWidget->item(r, kVisible);
        if (item != nullptr && id == item->data(kIdRole).toUInt())
        {
            return r;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::LayerGroupRow(vidtype groupId) const -> int
{
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r)
    {
        if (KindOfRow(r) != RowKind::LayerGroup)
        {
            continue;
        }

        const QTableWidgetItem *item = ui->tableWidget->item(r, kVisible);
        if (item != nullptr && groupId == item->data(kIdRole).toUInt())
        {
            return r;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::LayerNames() const -> QMap<quint32, QString>
{
    QMap<quint32, QString> names;

    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        names.insert(it.key(), it.value().name);
    }

    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::CandidateLayers(quint32 groupId) const -> QMap<quint32, QString>
{
    QMap<quint32, QString> candidates;

    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        // The layers no group holds, and — when editing — the ones this group already holds: a candidate list built
        // from the free layers alone would empty the group the moment its dialog opened.
        if (it.value().layerGroup == NULL_ID || (groupId != NULL_ID && it.value().layerGroup == groupId))
        {
            candidates.insert(it.key(), it.value().name);
        }
    }

    return candidates;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionAddLayer()
{
    // The dialog collects a name, a group and the layer's own pen here: the new layer is empty, its objects are
    // added afterwards from the canvas. A throwaway container is enough to build it.
    QScopedPointer<VContainer> const fackeContainer(new VContainer(VAbstractApplication::VApp()->TrVars(),
                                                                   VAbstractValApplication::VApp()->patternUnitsP(),
                                                                   VContainer::UniqueNamespace()));
    QScopedPointer<DialogGroup> const dialog(new DialogGroup(fackeContainer.data(), m_doc, NULL_ID, this));
    dialog->SetLayerGroups(m_doc->LayerGroupNames());

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    const quint32 newId = m_pattern->getNextId();
    const quint32 previous = m_doc->GetActiveGroup();

    // The layer just created becomes the active one — it is where the next object will land. Every write goes into
    // a single undo step, so one Ctrl+Z takes the layer away entirely — and the group it brought with it.
    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("add a layer"));

    // A group named in the dialog is created here, before the layer: the layer is written with the group from the
    // start, because CreateGroup hands back an element that is not in the document yet — nothing could look it up.
    const quint32 layerGroup = MaterializeLayerGroup(dialog->GetLayerGroup(), dialog->GetNewLayerGroupName());

    if (const QDomElement group = m_doc->CreateGroup(newId,
                                                     dialog->GetName(),
                                                     {},
                                                     null_id,
                                                     dialog->GetColor(),
                                                     dialog->GetPenStyle(),
                                                     layerGroup);
        !group.isNull())
    {
        auto *addGroup = new AddGroup(group, m_doc);
        connect(addGroup, &AddGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        undoStack->push(addGroup);

        auto *change = new ChangeActiveGroup(m_doc, newId, previous);
        connect(change, &ChangeActiveGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        undoStack->push(change);
    }

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionEditLayer(quint32 id)
{
    QScopedPointer<VContainer> const fackeContainer(new VContainer(VAbstractApplication::VApp()->TrVars(),
                                                                   VAbstractValApplication::VApp()->patternUnitsP(),
                                                                   VContainer::UniqueNamespace()));
    QScopedPointer<DialogGroup> const dialog(new DialogGroup(fackeContainer.data(), m_doc, NULL_ID, this));
    dialog->SetName(m_doc->GetGroupName(id));
    dialog->SetLayerGroups(m_doc->LayerGroupNames());
    dialog->SetLayerGroup(m_doc->GetLayerGroup(id));
    dialog->SetColor(m_doc->GetGroupColor(id));
    dialog->SetPenStyle(m_doc->GetGroupPenStyle(id));

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("edit layer"));

    const quint32 layerGroup = MaterializeLayerGroup(dialog->GetLayerGroup(), dialog->GetNewLayerGroupName());

    auto *changeGroupOptions = new ChangeGroupOptions(m_doc, id, dialog->GetName(), layerGroup, dialog->GetColor(),
                                                      dialog->GetPenStyle());
    connect(changeGroupOptions, &ChangeGroupOptions::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    // Through the document too: the canvas chrome (the rule's colour, the layer's name) is refreshed by
    // MainWindow, which listens to the document and not to this dock.
    connect(changeGroupOptions, &ChangeGroupOptions::UpdateGroups, m_doc,
            &VAbstractPattern::UpdateVisiblityGroups);
    undoStack->push(changeGroupOptions);

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::MaterializeLayerGroup(quint32 choice, const QString &newName) -> quint32
{
    if (choice != VAbstractPattern::NewLayerGroupId)
    {
        return choice; // NULL_ID for "no group", or a group that already exists
    }

    if (newName.isEmpty())
    {
        return NULL_ID;
    }

    const quint32 id = m_pattern->getNextId();

    const QDomElement element = m_doc->CreateLayerGroup(id, newName);
    if (element.isNull())
    {
        return NULL_ID;
    }

    auto *addGroup = new AddLayerGroup(element, m_doc);
    connect(addGroup, &AddLayerGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(addGroup);

    return id;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionAddLayerGroup()
{
    QScopedPointer<OLayerGroupDialog> const dialog(new OLayerGroupDialog(this));
    dialog->SetTakenNames(m_doc->LayerGroupNames().values());
    // A group is filled with the layers no other group holds. It may be created empty: the dialog reopens on it.
    dialog->SetLayers(CandidateLayers(NULL_ID), {});

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    const quint32 id = m_pattern->getNextId();

    const QDomElement element = m_doc->CreateLayerGroup(id, dialog->GroupName());
    if (element.isNull())
    {
        return;
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("add a group of layers"));

    auto *addGroup = new AddLayerGroup(element, m_doc);
    connect(addGroup, &AddLayerGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    undoStack->push(addGroup);

    if (const QVector<quint32> layers = dialog->Layers(); not layers.isEmpty())
    {
        auto *change = new ChangeLayersGroup(m_doc, layers, id);
        connect(change, &ChangeLayersGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
        undoStack->push(change);
    }

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionEditLayerGroup(quint32 id)
{
    QStringList taken;
    const QMap<quint32, QString> groups = m_doc->LayerGroupNames();
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        if (it.key() != id)
        {
            taken.append(it.value());
        }
    }

    QScopedPointer<OLayerGroupDialog> const dialog(new OLayerGroupDialog(this));
    dialog->SetTakenNames(taken);
    dialog->SetGroupName(m_doc->GetLayerGroupName(id));
    dialog->SetLayers(CandidateLayers(id), m_doc->LayerGroupLayers(id));

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    auto *change = new ChangeLayerGroupOptions(m_doc, id, dialog->GroupName(), dialog->Layers());
    connect(change, &ChangeLayerGroupOptions::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(change);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionDeleteLayerGroup(quint32 id)
{
    // A group holding layers is not deletable — it has to be emptied first, which the transfer button does. The icon
    // stays clickable so the refusal can be explained: a dimmed icon says "no" without telling why.
    if (const QVector<quint32> content = m_doc->LayerGroupLayers(id); not content.isEmpty())
    {
        const int count = static_cast<int>(content.size());
        QMessageBox::information(this, tr("Group not empty"),
                                 tr("The group \"%1\" still holds %n layer(s). Move them to another group first, "
                                    "(the transfer button), then delete the group.",
                                    "", count)
                                     .arg(m_doc->GetLayerGroupName(id)));
        return;
    }

    auto *delGroup = new DelLayerGroup(m_doc, id);
    connect(delGroup, &DelLayerGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(delGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionTransferLayerGroup(quint32 id)
{
    QMap<quint32, QString> content;
    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        if (it.value().layerGroup == id)
        {
            content.insert(it.key(), it.value().name);
        }
    }

    if (content.isEmpty())
    {
        QMessageBox::information(this, tr("Empty group"),
                                 tr("The group \"%1\" holds no layer - there is nothing to transfer.")
                                     .arg(m_doc->GetLayerGroupName(id)));
        return;
    }

    // The group itself is not a destination: moving its layers into the group they are already in would move
    // nothing. "No group" is one, and it is how a group is emptied.
    QMap<quint32, QString> targets = m_doc->LayerGroupNames();
    targets.remove(id);

    QScopedPointer<OTransferLayersDialog> const dialog(new OTransferLayersDialog(this));
    dialog->SetLayers(content);
    dialog->SetTargets(targets);

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    auto *change = new ChangeLayersGroup(m_doc, dialog->SelectedLayers(), dialog->TargetGroup());
    connect(change, &ChangeLayersGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(change);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ToggleLayerGroup(quint32 id)
{
    auto *change = new ChangeLayerGroupCollapsed(m_doc, id, not m_doc->GetLayerGroupCollapsed(id));
    connect(change, &ChangeLayerGroupCollapsed::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(change);
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::LayerGroupFullyVisible(quint32 groupId) const -> bool
{
    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    bool found = false;

    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        if (it.value().layerGroup != groupId)
        {
            continue;
        }

        if (not it.value().visible)
        {
            return false;
        }

        found = true;
    }

    // An empty group has nothing to hide, and reads as shown.
    return found;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::SetLayerGroupVisibility(quint32 groupId, bool visible)
{
    QVector<vidtype> layers;
    const QMap<quint32, VGroupData> all = m_doc->GetGroups();
    for (auto it = all.constBegin(); it != all.constEnd(); ++it)
    {
        if (it.value().layerGroup == groupId)
        {
            layers.append(it.key());
        }
    }

    if (layers.isEmpty())
    {
        // Said rather than done silently: the eye of an empty group cannot do what it promises.
        emit ShowStatusTip(tr("The group \"%1\" holds no layer yet - there is nothing to show or hide.")
                               .arg(m_doc->GetLayerGroupName(groupId)));
        return;
    }

    SetMultipleGroupsVisibility(layers, visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::RefreshGroupEyes() const
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        if (KindOfRow(row) != RowKind::LayerGroup)
        {
            continue;
        }

        QTableWidgetItem *item = ui->tableWidget->item(row, kVisible);
        if (item == nullptr)
        {
            continue;
        }

        const quint32 id = item->data(kIdRole).toUInt();

        bool anyVisible = false;
        for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
        {
            if (it.value().layerGroup == id && it.value().visible)
            {
                anyVisible = true;
                break;
            }
        }

        item->setIcon(MakeCellIcon(anyVisible ? QStringLiteral("eye") : QStringLiteral("eye-closed"), dark));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionDeleteLayer(quint32 id)
{
    // A layer holding objects is not deletable — it must be emptied first. The icon stays clickable
    // so the refusal can be explained: a dimmed icon says "no" without telling why.
    //
    // What counts as content is settled in ONE place, LayerItems: an `<item>` naming a tool the document no
    // longer has is not content, and it is not offered by "+", "−" or "↗" either — all three are fed by
    // LayerItems. Counting the raw entries here made the dock contradict itself: a layer whose only entry was
    // a leftover was refused the deletion ("still contains 1 object") while the transfer answered "there is
    // nothing to transfer". A dead end with no way out — measured on a real file.
    const QVector<QPair<quint32, quint32>> content = LayerItems(id);
    if (not content.isEmpty())
    {
        const int count = static_cast<int>(content.size());
        QMessageBox::information(this, tr("Layer not empty"),
                                 tr("The layer \"%1\" still contains %n object(s). Remove them from the layer "
                                    "first, then delete it.",
                                    "", count)
                                     .arg(m_doc->GetGroupName(id)));
        return;
    }

    // A piece always keeps an active layer, and so at least one layer. With none, the flag would have nowhere to
    // go and every object drawn after that would belong to no layer — the invariant the layers exist for. Said
    // rather than done silently: a layer appearing on its own after a deletion would look like a failed deletion.
    if (m_doc->GetGroups().size() < 2)
    {
        QMessageBox::information(this, tr("Last layer"),
                                 tr("The layer \"%1\" is the only one of this piece. An object must always belong "
                                    "to a layer, so a piece keeps at least one - add another layer first, then "
                                    "delete this one.").arg(m_doc->GetGroupName(id)));
        return;
    }

    auto *delGroup = new DelGroup(m_doc, id);
    connect(delGroup, &DelGroup::UpdateGroups, this, &VWidgetGroups::UpdateGroups);
    VAbstractApplication::VApp()->getUndoStack()->push(delGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ActionHideAll()
{
    // Walked from the DOCUMENT, not from the table: the layers of a folded group are not listed, and both commands
    // are about the piece as a whole — a hidden layer inside a folded group has to be reachable from here.
    QVector<vidtype> groups;
    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    groups.reserve(layers.size());
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        if (it.value().visible)
        {
            groups.append(it.key());
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
    QVector<vidtype> groups;
    const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
    groups.reserve(layers.size());
    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        if (not it.value().visible)
        {
            groups.append(it.key());
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
    const quint32 id = idItem->data(kIdRole).toUInt();
    const RowKind kind = KindOfRow(row);

    // Taken before the action, not after: what the action rebuilds may already have moved the list (see
    // SelectActiveRow). The eye, the gear and the trash all come back here, and all three must leave the list
    // where the reader is looking.
    RememberScroll();

    if (kind == RowKind::LayerGroup)
    {
        switch (column)
        {
            case kVisible:
                // All shown hides, anything else shows: one click always lands on a state the reader asked for,
                // and undoing a hide is the same gesture.
                SetLayerGroupVisibility(id, not LayerGroupFullyVisible(id));
                SelectActiveRow(true);
                break;
            case kName:
                // The caret before the name folds and unfolds the group — the whole line answers, like the
                // measurement groups. Everything that follows the line belongs to it.
                ToggleLayerGroup(id);
                SelectActiveRow(true);
                break;
            case kEdit:
                ActionEditLayerGroup(id);
                SelectActiveRow(true);
                break;
            case kTransfer:
                ActionTransferLayerGroup(id);
                break;
            case kDelete:
                ActionDeleteLayerGroup(id);
                SelectActiveRow(true);
                break;
            default:
                break;
        }

        return;
    }

    switch (column)
    {
        case kVisible:
            SetGroupVisibility(id, not m_doc->GetGroupVisibility(id));
            // The highlight belongs to the active layer, never to the last click — but the list itself does not
            // move: a long one has to stay where the reader is looking (see SelectActiveRow).
            SelectActiveRow(true);
            break;
        case kName:
            // A single click on the name makes the layer active. A double click still renames it: the editor
            // opens on top of the same gesture, and RenameGroup only fires on the committed text.
            SetActiveLayer(id);
            break;
        case kEdit:
            ActionEditLayer(id);
            // After the dialog, so the position survives the rebuild the accepted change triggers.
            SelectActiveRow(true);
            break;
        case kDelete:
            ActionDeleteLayer(id);
            SelectActiveRow(true);
            break;
        case kAdd:
            // Picking objects for this layer: the canvas does the clicking from now on, Enter confirms.
            StartObjectSelection(id, ObjectSelection::Adding);
            break;
        case kTransfer:
            // The same gesture again, but it ends on a question: which layer the picked objects move to.
            StartObjectSelection(id, ObjectSelection::Transferring);
            break;
        case kRemove:
            // The other direction, and the same gesture. What a removal may take out is narrowed to the objects
            // another layer also holds — see RemovableObjects.
            StartObjectSelection(id, ObjectSelection::Removing);
            break;
        default:
            SelectActiveRow(true);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::RefusalReason(quint32 layerId, ObjectSelection mode) -> QString
{
    if (mode == ObjectSelection::Transferring && m_doc->GetGroups().size() < 2)
    {
        return tr("This is the only layer of this piece - there is no other layer to transfer to.");
    }

    if (mode != ObjectSelection::Adding && LayerItems(layerId).isEmpty())
    {
        return mode == ObjectSelection::Transferring
                   ? tr("This layer holds no object - there is nothing to transfer.")
                   : tr("This layer holds no object - there is nothing to remove.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::StartObjectSelection(quint32 layerId, ObjectSelection mode)
{
    // Whatever was being picked is dropped first: the marks live on the canvas, and the new pass knows nothing
    // about them — a layer left half-picked would keep its orange for good.
    CancelObjectSelection();

    const QVector<QPair<quint32, quint32>> layerItems = LayerItems(layerId);

    m_selectionLayerId = layerId;
    m_objectSelection = mode;

    for (const auto &[objectId, toolId] : layerItems)
    {
        m_layerObjects.insert(objectId, toolId);
    }

    if (mode == ObjectSelection::Removing)
    {
        m_candidateObjects = RemovableObjects(layerItems);
    }
    else if (mode == ObjectSelection::Transferring)
    {
        m_candidateObjects = TransferableObjects(layerItems);
    }

    // A picking that can lead nowhere is ARMED all the same, and that is the whole point of it: armed, the dock
    // owns every click on an object — the canvas never falls back on its ordinary selection, so nothing lights up
    // that the layer cannot act on. The sentence says why nothing will come of it, and Escape lets go of it.
    const QString reason = RefusalReason(layerId, mode);

    // Magenta says what a click may pick, and nothing else. For an addition that is the layer's CONTENT — the
    // objects already in it, which cannot be added a second time. For the two other directions it is the
    // CANDIDATE set: an object no other layer also holds cannot be removed, so marking it would promise a click
    // the invariant refuses; and a picking with nowhere to go — the only layer of the piece — can pick nothing
    // at all, so it marks nothing. What is marked is remembered, and given back as it was (see CancelObjectSelection).
    m_markedObjects.clear();
    if (reason.isEmpty())
    {
        m_markedObjects = mode == ObjectSelection::Adding ? m_layerObjects : m_candidateObjects;
        for (auto it = m_markedObjects.constBegin(); it != m_markedObjects.constEnd(); ++it)
        {
            emit ObjectInLayerChanged(it.key(), it.value(), true);
        }
    }

    emit ObjectSelectionStarted();

    if (not reason.isEmpty())
    {
        emit ShowStatusTip(reason);
    }
    else
    {
        UpdateSelectionTip();
    }

    SelectActiveRow(true);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ToggleLayerObjects(const QVector<QPair<quint32, quint32>> &objects)
{
    if (not IsSelectingObjects() || objects.isEmpty())
    {
        return;
    }

    // Adding is the one direction where the click itself decides what may be picked — everything the layer does
    // not hold yet. The other two start from the layer's own content: a removal keeps what another layer holds
    // too, a transfer keeps every object of it that can be drawn.
    const bool adding = m_objectSelection == ObjectSelection::Adding;

    QVector<QPair<quint32, quint32>> toggled;
    toggled.reserve(objects.size());
    for (const auto &object : objects)
    {
        const bool pickable = adding ? m_layerObjects.value(object.first, NULL_ID) != object.second
                                     : m_candidateObjects.value(object.first, NULL_ID) == object.second;
        if (pickable)
        {
            toggled.append(object);
        }
    }

    if (toggled.isEmpty())
    {
        if (adding)
        {
            ExplainAddRefusal();
        }
        else
        {
            ExplainRefusal(objects.constFirst());
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
auto VWidgetGroups::RealItems(const VGroupData &layer) -> QVector<QPair<quint32, quint32>>
{
    // What the layer holds AND what the document can still show. The two are not the same: an `<item>` may name
    // a tool the file no longer has, and real files carry such entries — a group written by Valentina whose tool
    // was deleted without its entry being cleaned (measured: `<item object="75" tool="75"/>` in a pattern holding
    // no element of that id anywhere). This is the ONE place that settles what a layer's content is, because a
    // kept entry is not merely undrawable: `getTool()` THROWS VExceptionBadId on an id the document does not know
    // (vabstractpattern.cpp, ToolExists), and the marking emits from inside a Qt signal — where an exception that
    // escapes aborts the whole application. That was a real crash, from a layer holding exactly one such entry.
    //
    // The OBJECT is checked too, and not only its tool: re-editing an operation's options replaces its element
    // (SaveToolOptions), and a source that goes away takes its destination's id with it — the layer then keeps a
    // pair naming an object nobody draws. The loading pass removes those from the file (CleanStaleLayerItems, on
    // the same model rule); here is where the dock stops counting, showing and offering them.
    QVector<QPair<quint32, quint32>> items;
    items.reserve(layer.items.size());
    for (const auto &item : layer.items)
    {
        if (VAbstractPattern::FindTool(item.second) == nullptr)
        {
            continue;
        }

        // Asked by id and not from the element, because the element is not always there: the line an operation
        // produces has none — the operation keeps its id and the parse rebuilds it — and it is a line all the
        // same. VAbstractPattern::LayerableObjectsOfTool answers for it, which is exactly what the loading pass
        // assumes when it keeps the entry in the file.
        //
        // A tool the model cannot enumerate — a pattern piece — keeps its entry rather than being thrown away:
        // that is what the loading pass does with it, and the dock has no business saying otherwise.
        const QVector<QPair<quint32, quint32>> produced = m_doc->LayerableObjectsOfTool(item.second);
        if (not produced.isEmpty() && not produced.contains(item))
        {
            continue;
        }

        items.append(item);
    }

    return items;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::LayerItems(quint32 layerId) -> QVector<QPair<quint32, quint32>>
{
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    const auto layer = groups.constFind(layerId);
    if (layer == groups.constEnd())
    {
        return {};
    }

    return RealItems(layer.value());
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::RemovableObjects(const QVector<QPair<quint32, quint32>> &items) -> QMap<quint32, quint32>
{
    // How many layers of this piece hold each object. Counted over the whole piece, not just the layer: the
    // question is precisely whether something ELSE holds it. A layer the tag filter hides counts like any other
    // — it is a real layer, and it really does hold the object. A LEFTOVER entry does not: counting one as a
    // holder would let a removal take an object out of its last real layer, which is the very thing this test
    // exists to prevent.
    QMap<QPair<quint32, quint32>, int> holders;
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    for (auto group = groups.constBegin(); group != groups.constEnd(); ++group)
    {
        for (const auto &item : RealItems(group.value()))
        {
            ++holders[item];
        }
    }

    // Drawn first, shared second: what cannot be drawn cannot be clicked, whatever the count says.
    QMap<quint32, quint32> removable;
    const QMap<quint32, quint32> drawable = TransferableObjects(items);
    for (auto it = drawable.constBegin(); it != drawable.constEnd(); ++it)
    {
        if (holders.value(QPair<quint32, quint32>(it.key(), it.value())) >= 2)
        {
            removable.insert(it.key(), it.value());
        }
    }

    return removable;
}

//---------------------------------------------------------------------------------------------------------------------
auto VWidgetGroups::TransferableObjects(const QVector<QPair<quint32, quint32>> &items) -> QMap<quint32, quint32>
{
    // Every object the layer holds may leave it — unlike a removal, a transfer writes into the target BEFORE it
    // takes out of the source, so the invariant never bites here. Which objects a layer holds at all, and which
    // of them the document can still draw, is settled once in LayerItems; this only names the rule.
    QMap<quint32, quint32> transferable;

    for (const auto &item : items)
    {
        transferable.insert(item.first, item.second);
    }

    return transferable;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::UpdateSelectionTip()
{
    // A picking the layer cannot answer says why, whatever gets picked in the meantime: Enter will refuse it, and
    // promising "Enter to choose the layer they move to" on the only layer of a piece would be a lie.
    if (const QString reason = RefusalReason(m_selectionLayerId, m_objectSelection); not reason.isEmpty())
    {
        emit ShowStatusTip(reason);
        return;
    }

    const int count = static_cast<int>(m_selectedObjects.size());
    if (count > 0)
    {
        // A transfer has one more question to ask, and "confirm" there would promise a move the dialog can still
        // call off.
        emit ShowStatusTip(m_objectSelection == ObjectSelection::Transferring
                               ? tr("%n object(s) selected - Enter to choose the layer they move to, Escape cancels.",
                                    "", count)
                               : tr("%n object(s) selected - Enter to confirm, Escape cancels.", "", count));
        return;
    }

    if (m_objectSelection == ObjectSelection::Transferring)
    {
        // Nothing to say here about the invariant that holds for a removal: a transfer writes into the target
        // before it takes out of the source, so every object the layer holds may leave it.
        emit ShowStatusTip(tr("Select the objects to transfer, then press Enter to choose the layer they move to - "
                              "Escape cancels. Magenta is this layer's content, and every object of it can move."));
        return;
    }

    if (m_objectSelection == ObjectSelection::Removing)
    {
        // The invariant, spelled out, and said again on every pass: it is the only reason a click can be refused.
        if (m_candidateObjects.isEmpty())
        {
            emit ShowStatusTip(tr("No object of this layer is shared with another layer - an object must always "
                                  "belong to at least one layer, so there is nothing to remove here."));
            return;
        }

        emit ShowStatusTip(tr("Select the objects to remove, then press Enter to confirm - Escape cancels. Magenta "
                              "is what can be removed: an object must always stay in one layer, so only what "
                              "another layer holds can go."));
        return;
    }

    emit ShowStatusTip(tr("Select the objects to add, then press Enter to confirm - Escape cancels. Magenta is this "
                          "layer's content, so those cannot be added again."));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ExplainRefusal(const QPair<quint32, quint32> &object)
{
    // A transfer puts the object into the target before it takes it out of the source, so nothing of the layer is
    // refused for the invariant: the only refusal left is a click that landed outside the layer.
    if (m_objectSelection == ObjectSelection::Transferring)
    {
        emit ShowStatusTip(tr("This object is not in this layer."));
        return;
    }

    // Two different refusals, and saying the wrong one would send the user looking for a layer that is not there.
    const QMap<quint32, QString> holders = m_doc->GetGroupsContainingItem(object.second, object.first, true);
    if (holders.contains(m_selectionLayerId))
    {
        emit ShowStatusTip(tr("This object belongs only to this layer - an object must always belong to at least "
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
    emit ShowStatusTip(tr("This object is already in this layer - a layer holds an object once."));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::ValidateObjectSelection()
{
    if (not IsSelectingObjects())
    {
        return;
    }

    const quint32 layerId = m_selectionLayerId;

    // Nothing to carry out: say again why, and write nothing. Enter on a picking the layer cannot answer must not
    // open a dialog, nor close a macro on nothing.
    if (const QString reason = RefusalReason(layerId, m_objectSelection); not reason.isEmpty())
    {
        emit ShowStatusTip(reason);
        return;
    }

    // Armed, but nothing picked yet: Enter has nothing to carry out whatever the direction, and asking where to
    // move nothing would be a dialog for nothing.
    if (m_selectedObjects.isEmpty())
    {
        UpdateSelectionTip();
        return;
    }

    // A transfer asks its question before anything is written, and a dismissed dialog writes nothing: the picking
    // stays armed so Enter can ask again, and Escape is still there to let go of it.
    if (m_objectSelection == ObjectSelection::Transferring)
    {
        TransferSelectedObjects(layerId);
        return;
    }

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
auto VWidgetGroups::AskTargetLayer(quint32 sourceId) -> quint32
{
    const QMap<quint32, VGroupData> groups = m_doc->GetGroups();
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    OChooseLayerDialog::Config config;
    config.acceptText = tr("Transfer");
    // The source layer is named: the dock keeps the highlight on the ACTIVE layer, which is not necessarily this
    // one, so "this layer" would not say which layer the objects are leaving.
    config.intro = tr("%n object(s) will leave \"%1\". Choose the layer they move to:", "",
                      static_cast<int>(m_selectedObjects.size()))
                       .arg(groups.value(sourceId).name);

    for (auto group = groups.constBegin(); group != groups.constEnd(); ++group)
    {
        // Every layer but the source, hidden ones included: the tag filter only hides rows from this dock, and a
        // removal already counts a filtered-out layer as a real holder for exactly the same reason.
        if (group.key() == sourceId)
        {
            continue;
        }

        config.choices.append({group.key(), group.value().name, MakeColorDot(group.value().color, dark)});
    }
    config.preselected = m_doc->GetActiveGroup();

    OChooseLayerDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return NULL_ID;
    }

    return dialog.ChosenLayerId();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::TransferSelectedObjects(quint32 sourceId)
{
    if (m_selectedObjects.isEmpty())
    {
        // Enter with nothing picked leaves the picking, exactly as it does in the two other directions.
        CancelObjectSelection();
        UpdateGroups();
        return;
    }

    const quint32 targetId = AskTargetLayer(sourceId);
    if (targetId == NULL_ID)
    {
        return; // the dialog was dismissed: the picking is left exactly as it was
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("transfer objects to another layer"));

    for (auto it = m_selectedObjects.constBegin(); it != m_selectedObjects.constEnd(); ++it)
    {
        // The target first, the source second: at no moment does the object belong to no layer. One command per
        // write, and both are the single writers the rest of the layers work already goes through.
        if (const QDomElement added = m_doc->AddItemToGroup(it.value(), it.key(), targetId); not added.isNull())
        {
            undoStack->push(new ::AddItemToGroup(added, m_doc, targetId));
        }

        // An object the target already holds writes nothing on the way in — the writer refuses a double — so all
        // that is left to do for it is to take it out of the source.
        if (const QDomElement removed = m_doc->RemoveItemFromGroup(it.value(), it.key(), sourceId);
            not removed.isNull())
        {
            undoStack->push(new ::RemoveItemFromGroup(removed, m_doc, sourceId));
        }
    }

    undoStack->endMacro();

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

    // The selection first — in a removal the selected objects are also part of the magenta set, so they have to go
    // back to magenta before that set is given back its own colour.
    for (auto it = m_selectedObjects.constBegin(); it != m_selectedObjects.constEnd(); ++it)
    {
        emit ObjectSelectedChanged(it.key(), it.value(), false);
    }

    // Exactly what was marked, and not the layer's content recomputed: the two are not the same in a removal,
    // where only the candidates wear magenta, and not even the same shape when the picking could lead nowhere.
    for (auto it = m_markedObjects.constBegin(); it != m_markedObjects.constEnd(); ++it)
    {
        emit ObjectInLayerChanged(it.key(), it.value(), false);
    }

    m_selectedObjects.clear();
    m_layerObjects.clear();
    m_candidateObjects.clear();
    m_markedObjects.clear();
    m_selectionLayerId = NULL_ID;
    m_objectSelection = ObjectSelection::Idle;
    emit ObjectSelectionFinished();
    // The sentence goes with the picking it described — including the one a refusal left: a refusal armed the
    // picking rather than being a state of its own, so letting go of it takes the sentence away as well.
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
void VWidgetGroups::SelectActiveRow(bool keepScroll)
{
    const int row = GroupRow(m_doc->GetActiveGroup());
    if (row == -1)
    {
        ui->tableWidget->clearSelection();
        return;
    }

    // Selecting a row scrolls it into view, and the active layer is often the last one of the list: on a long list,
    // clicking the eye of a layer near the top threw the whole dock back to the bottom. Where the user is looking is
    // where they stay — the row comes back into the selection, not into view. Both bars, because a row selection
    // also brings the first column back into sight. The position is the one taken when the action began, not the one
    // the rebuild left behind: editing a layer goes through the document, and that rebuild already scrolled the list.
    QScrollBar *vertical = ui->tableWidget->verticalScrollBar();
    QScrollBar *horizontal = ui->tableWidget->horizontalScrollBar();
    const int scrollY = keepScroll ? m_scrollY : 0;
    const int scrollX = keepScroll ? m_scrollX : 0;

    ui->tableWidget->selectRow(row);

    if (keepScroll)
    {
        vertical->setValue(scrollY);
        horizontal->setValue(scrollX);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::RememberScroll()
{
    m_scrollY = ui->tableWidget->verticalScrollBar()->value();
    m_scrollX = ui->tableWidget->horizontalScrollBar()->value();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::RenameGroup(int row, int column)
{
    // Only a layer's name is edited in place: a group row carries no editor (its name is changed in its dialog),
    // and renaming the layer a group row would be taken for is exactly what that guard prevents.
    if (column != kName || KindOfRow(row) != RowKind::Layer)
    {
        return;
    }

    const QTableWidgetItem *idItem = ui->tableWidget->item(row, kVisible);
    if (idItem == nullptr)
    {
        return;
    }

    const quint32 id = idItem->data(kIdRole).toUInt();
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
    const quint32 id = item->data(kIdRole).toUInt();
    const RowKind kind = KindOfRow(row);
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    // Same rule as the left click: what the menu does to a layer must not move the list (see RememberScroll).
    RememberScroll();

    auto MultipleChangeVisibilityTo = [this](bool visibility)
    {
        const QMap<quint32, VGroupData> layers = m_doc->GetGroups();
        for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
        {
            if (visibility != it.value().visible)
            {
                return true;
            }
        }

        return false;
    };

    if (kind == RowKind::LayerGroup)
    {
        QScopedPointer<QMenu> const menu(new QMenu());
        QAction  const*actionEdit = menu->addAction(MakeCellIcon(QStringLiteral("gear-six"), dark), tr("Edit"));
        QAction  const*actionTransfer =
            menu->addAction(MakeCellIcon(QStringLiteral("arrow-bend-up-right"), dark), tr("Transfer"));
        QAction  const*actionDelete = menu->addAction(MakeCellIcon(QStringLiteral("trash"), dark), tr("Delete"));

        QAction  const*selectedAction = menu->exec(ui->tableWidget->viewport()->mapToGlobal(pos));

        if (selectedAction == actionEdit)
        {
            ActionEditLayerGroup(id);
            SelectActiveRow(true);
        }
        else if (selectedAction == actionTransfer)
        {
            ActionTransferLayerGroup(id);
        }
        else if (selectedAction == actionDelete)
        {
            ActionDeleteLayerGroup(id);
            SelectActiveRow(true);
        }

        return;
    }

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
        SelectActiveRow(true);
    }
    else if (selectedAction == actionEdit)
    {
        ActionEditLayer(id);
        SelectActiveRow(true);
    }
    else if (selectedAction == actionDelete)
    {
        ActionDeleteLayer(id);
        SelectActiveRow(true);
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

    // ⚠️ Les deux boutons, la case et les infobulles des lignes sont écrits en code, et `ui->retranslateUi` ne les
    // voit pas. `UpdateGroups` reconstruit la table : aucune connexion n'est rejouée, contrairement à un appel du
    // constructeur.
    m_addButton->setText(tr("Add a layer"));
    if (m_addGroupButton != nullptr)
    {
        m_addGroupButton->setText(tr("Add a group"));
    }
    m_showActiveLayer->setText(tr("Show the active layer on the canvas"));
    UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::FillTable(QMap<quint32, VGroupData> groups)
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QSignalBlocker blocker(ui->tableWidget);

    // The field filters on the layers' names, and completes on them.
    const QString filter = ui->lineEditLayerFilter->text().simplified();
    ui->lineEditLayerFilter->SetCompletion(LayerNames().values());

    const QMap<quint32, VLayerGroupData> layerGroups = m_doc->GetLayerGroups();

    auto MatchesFilter = [&filter](const QString &name)
    { return filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive); };

    // The order the rows are READ in: the layers no group holds, then each group followed by its own layers. The
    // table is not sorted afterwards — the group rows ARE the structure, and a sort by name would scatter every
    // group's content through the top level. Both lists go by name, so the same piece always reads the same way.
    auto SortedIds = [](QVector<QPair<QString, quint32>> entries) -> QVector<quint32>
    {
        std::sort(entries.begin(), entries.end(),
                  [](const QPair<QString, quint32> &left, const QPair<QString, quint32> &right)
                  {
                      const int order = QString::compare(left.first, right.first, Qt::CaseInsensitive);
                      return order != 0 ? order < 0 : left.second < right.second;
                  });

        QVector<quint32> ids;
        ids.reserve(entries.size());
        for (const auto &entry : entries)
        {
            ids.append(entry.second);
        }
        return ids;
    };

    // 1. The layers no group holds, at the top — a group row owns everything that follows it, so a layer outside
    //    every group has nowhere else to be.
    QVector<QPair<QString, quint32>> free;
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        if (it.value().layerGroup == NULL_ID && MatchesFilter(it.value().name))
        {
            free.append({it.value().name, it.key()});
        }
    }

    struct RowDesc
    {
        RowKind kind;
        quint32 id;
    };

    QVector<RowDesc> rows;
    for (const quint32 id : SortedIds(free))
    {
        rows.append({RowKind::Layer, id});
    }

    // 2. Each group, then its layers. A group is kept when it has something to show: its own name matches (then
    //    ALL its layers are shown, the filter is not about them) or at least one of its layers does. Filtering
    //    unfolds the groups — a match hidden inside a folded group would be a search that answers nothing.
    QVector<QPair<QString, quint32>> sortedGroups;
    for (auto it = layerGroups.constBegin(); it != layerGroups.constEnd(); ++it)
    {
        sortedGroups.append({it.value().name, it.key()});
    }

    for (const quint32 groupId : SortedIds(sortedGroups))
    {
        const VLayerGroupData group = layerGroups.value(groupId);
        const bool wholeGroup = MatchesFilter(group.name) && not filter.isEmpty();

        QVector<QPair<QString, quint32>> members;
        for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
        {
            if (it.value().layerGroup != groupId)
            {
                continue;
            }

            if (wholeGroup || MatchesFilter(it.value().name))
            {
                members.append({it.value().name, it.key()});
            }
        }

        if (not filter.isEmpty() && members.isEmpty())
        {
            continue;
        }

        const bool folded = group.collapsed && filter.isEmpty();

        rows.append({RowKind::LayerGroup, groupId});

        if (not folded)
        {
            for (const quint32 id : SortedIds(members))
            {
                rows.append({RowKind::Layer, id});
            }
        }
    }

    // `clearContents`, not `clear`: keep the header configuration (column widths and resize
    // modes) set up by the constructor.
    ui->tableWidget->clearContents();
    ui->tableWidget->setColumnCount(kColumns);
    ui->tableWidget->setRowCount(static_cast<int>(rows.size()));

    for (int row = 0; row < rows.size(); ++row)
    {
        if (rows.at(row).kind == RowKind::LayerGroup)
        {
            const quint32 id = rows.at(row).id;

            QVector<quint32> members;
            bool anyVisible = false;
            for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
            {
                if (it.value().layerGroup == id)
                {
                    members.append(it.key());
                    anyVisible = anyVisible || it.value().visible;
                }
            }

            FillLayerGroupRow(row, id, layerGroups.value(id), members, anyVisible, dark);
        }
        else if (const auto it = groups.constFind(rows.at(row).id); it != groups.constEnd())
        {
            FillLayerRow(row, it.key(), it.value(), dark);
        }
    }

    SelectActiveRow();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::FillLayerRow(int row, quint32 id, const VGroupData &data, bool dark)
{
    auto *visibleItem = new QTableWidgetItem(
        MakeCellIcon(data.visible ? QStringLiteral("eye") : QStringLiteral("eye-closed"), dark), QString());
    visibleItem->setData(kIdRole, id);
    visibleItem->setData(kKindRole, static_cast<quint32>(RowKind::Layer));
    // set the item non-editable (view only)
    visibleItem->setFlags(visibleItem->flags() & ~Qt::ItemIsEditable);
    visibleItem->setToolTip(tr("Show or hide the layer"));
    ui->tableWidget->setItem(row, kVisible, visibleItem);

    // The layer's colour, so a glance at the list tells which pen its objects are drawn with.
    auto *colorItem = new QTableWidgetItem(MakeColorDot(data.color, dark), QString());
    colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable);
    colorItem->setToolTip(tr("The colour the layer draws with"));
    ui->tableWidget->setItem(row, kColor, colorItem);

    auto *nameItem = new QTableWidgetItem(data.name);
    nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // The tooltip says where the layer sits: the row itself cannot show it without indenting the name, and the
    // list is read as one column. A layer outside every group says what a click on its name does, so that the
    // cell never falls back to the tooltip of the whole dock.
    if (const quint32 groupId = data.layerGroup; groupId != NULL_ID)
    {
        nameItem->setToolTip(tr("Group: %1.").arg(m_doc->GetLayerGroupName(groupId)));
    }
    else
    {
        nameItem->setToolTip(tr("Make this layer the active one"));
    }

    if (data.items.isEmpty())
    {
        QFont font = nameItem->font();
        font.setStrikeOut(true);
        nameItem->setFont(font);
    }

    ui->tableWidget->setItem(row, kName, nameItem);

    SetActionCell(ui->tableWidget, row, kEdit, MakeCellIcon(QStringLiteral("gear-six"), dark),
                  tr("Rename the layer"));
    SetActionCell(ui->tableWidget, row, kAdd, MakeCellIcon(QStringLiteral("plus"), dark),
                  tr("Add objects to the layer"));
    // The bend arrow: an object changes layer without ever leaving every layer — the icon the library draws,
    // not one this dock invented.
    SetActionCell(ui->tableWidget, row, kTransfer, MakeCellIcon(QStringLiteral("arrow-bend-up-right"), dark),
                  tr("Move objects of this layer to another one"));
    SetActionCell(ui->tableWidget, row, kRemove, MakeCellIcon(QStringLiteral("minus"), dark),
                  tr("Remove objects from the layer"));
    // The trash stays fully visible and clickable even on a layer holding objects: the refusal
    // is explained by a dialog (ActionDeleteLayer), not by a dimmed icon the reader cannot read.
    SetActionCell(ui->tableWidget, row, kDelete, MakeCellIcon(QStringLiteral("trash"), dark),
                  tr("Delete the layer - allowed once it holds no object"));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetGroups::FillLayerGroupRow(int row,
                                      quint32 id,
                                      const VLayerGroupData &group,
                                      const QVector<quint32> &layers,
                                      bool anyVisible,
                                      bool dark)
{
    auto *visibleItem = new QTableWidgetItem(
        MakeCellIcon(anyVisible ? QStringLiteral("eye") : QStringLiteral("eye-closed"), dark), QString());
    visibleItem->setData(kIdRole, id);
    visibleItem->setData(kKindRole, static_cast<quint32>(RowKind::LayerGroup));
    visibleItem->setFlags(visibleItem->flags() & ~Qt::ItemIsEditable);
    visibleItem->setToolTip(tr("Show or hide every layer of the group"));
    ui->tableWidget->setItem(row, kVisible, visibleItem);

    // No colour of its own: a group is not drawn with anything, its layers are.

    // The caret rides the name, like the measurement groups: what follows the line belongs to the group. The item
    // is NOT editable — a double click would otherwise open the layer's rename editor on a group row.
    auto *nameItem = new QTableWidgetItem(MakeCellIcon(group.collapsed ? QStringLiteral("caret-right")
                                                                      : QStringLiteral("caret-down"), dark),
                                          group.name);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameItem->setToolTip(tr("%n layer(s) in this group.", "", static_cast<int>(layers.size())));

    QFont font = nameItem->font();
    font.setBold(true);
    nameItem->setFont(font);

    ui->tableWidget->setItem(row, kName, nameItem);

    SetActionCell(ui->tableWidget, row, kEdit, MakeCellIcon(QStringLiteral("gear-six"), dark),
                  tr("Rename the group and settle the layers it holds"));
    SetActionCell(ui->tableWidget, row, kTransfer, MakeCellIcon(QStringLiteral("arrow-bend-up-right"), dark),
                  tr("Move layers of this group to another one"));
    SetActionCell(ui->tableWidget, row, kDelete, MakeCellIcon(QStringLiteral("trash"), dark),
                  tr("Delete the group - allowed once it holds no layer"));
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
