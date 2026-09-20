/************************************************************************
 **
 **  @file   owidgetsizestyles.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the "Size Styles" dock content.
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

#include "owidgetsizestyles.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QHeaderView>
#include <QIcon>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QVariantList>
#include <QVBoxLayout>

#include "core/ogratedsizestyles.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtheme/vtheme.h"

namespace
{

constexpr int kIconSize = 20;
// 1 pt = PrintDPI/72 px = 1.333 px at the app's fixed 96 dpi — same conversion the
// canvas render uses, so the dock preview matches the canvas motif at scale 1:1.
const qreal kPtToPx = 96.0 / 72.0;

// Table columns: Size | Name | Color (swatch) | Outline (preview).
constexpr int kSize = 0;
constexpr int kName = 1;
constexpr int kColor = 2;
constexpr int kOutline = 3;

// Per-cell roles carrying the style to the delegate (never the hex text in the cell).
constexpr int kColorRole = Qt::UserRole;     // "#RRGGBB" or "" (unstyled)
constexpr int kDashRole = Qt::UserRole + 1;  // QVariantList of qreal px [dash, space, ...]

//---------------------------------------------------------------------------------------------------------------------
auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

//---------------------------------------------------------------------------------------------------------------------
// One themed icon: Normal uses the current theme variant, Selected always uses
// the dark variant (accent/hover backgrounds are dark in both themes).
auto MakeCellIcon(const QString &baseName, bool dark) -> QIcon
{
    QIcon icon;
    icon.addFile(IconResPath(baseName, dark), QSize(), QIcon::Normal);
    icon.addFile(IconResPath(baseName, true), QSize(), QIcon::Selected);
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
// Draws the color swatch (column kColor) and the outline preview (kOutline). The
// base class paints nothing for those columns; a style is rendered exactly as it
// will appear on the canvas: same color, same dash motif — dashes are stored in
// pt and converted to px (kPtToPx) before being divided by the pen width.
class TraceDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        // No stored color = the default black trace (as on paper). Every row shows a
        // pastille and an outline preview, so a dash-only (or empty) style is still
        // visible — in black.
        QString colorText = index.data(kColorRole).toString();
        if (colorText.isEmpty())
        {
            colorText = QStringLiteral("#000000");
        }
        const QColor color(colorText);
        if (!color.isValid())
        {
            return;
        }

        const QPointF center = QRectF(option.rect).center();
        if (index.column() == kColor)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            constexpr qreal radius = 6.5;
            painter->drawEllipse(center, radius, radius);
            painter->restore();
            return;
        }

        // Outline preview: a trace drawn with the color and dashes across the WHOLE
        // column (minus a small side pad) — the line stretches with the dock width,
        // the dash sizes staying the same (more of the pattern repeats).
        const QVariantList dashData = index.data(kDashRole).toList();
        const qreal penWidth = 1.5;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(color, penWidth);
        QVector<qreal> dashes;
        dashes.reserve(dashData.size());
        for (const QVariant &value : dashData)
        {
            dashes.append(value.toReal());
        }
        // The very conversion the canvas and the export use, read at this widget's own resolution:
        // what the row shows is what the size will be drawn with, which is the whole point of the
        // column. A value with no style keeps a plain pen, and that pen is a solid one.
        OGradedSizeStyles::ApplyToPen(pen, color, dashes, kPtToPx);
        painter->setPen(pen);
        constexpr qreal kSidePad = 6.0;
        const qreal half = qMax(1.0, (option.rect.width() - 2.0 * kSidePad) / 2.0);
        painter->drawLine(QPointF(center.x() - half, center.y()),
                          QPointF(center.x() + half, center.y()));
        painter->restore();
    }
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OWidgetSizeStyles::OWidgetSizeStyles(QWidget *parent)
  : QWidget(parent)
{
    BuildUi();

    // The dock stays open across theme toggles — only the button icon is themed;
    // the list content is user data (never re-populated on a theme switch, so the
    // scroll position and the store reads are preserved).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]() {
        UpdateEditButtonIcon();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeStyles::BuildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    // ── "Edit size styles…" trigger (opens the styles dialog) ──
    m_editButton = new QPushButton(this);
    m_editButton->setObjectName(QStringLiteral("editSizeStylesButton"));
    m_editButton->setText(tr("Edit size styles"));
    m_editButton->setIconSize(QSize(kIconSize, kIconSize));
    m_editButton->setFocusPolicy(Qt::NoFocus);
    m_editButton->setAutoDefault(false);
    connect(m_editButton, &QPushButton::clicked, this, &OWidgetSizeStyles::editRequested);
    m_editButton->installEventFilter(this); // hover → dark glyph (accent background)
    rootLayout->addWidget(m_editButton, 0, Qt::AlignHCenter);

    // ── Global canvas gate (light/dark safe: trace colors target paper) ──
    m_showInCanvas = new QCheckBox(tr("Show in canvas"), this);
    m_showInCanvas->setObjectName(QStringLiteral("showSizeStylesInCanvas"));
    // Set the initial state BEFORE connecting so the toggled signal only fires on
    // real user changes (the MainWindow persists + applies then).
    m_showInCanvas->setChecked(
        VAbstractApplication::VApp()->Settings()->GetGradedSizeStylesShowInCanvas());
    connect(m_showInCanvas, &QCheckBox::toggled, this, &OWidgetSizeStyles::showInCanvasChanged);
    rootLayout->addWidget(m_showInCanvas, 0, Qt::AlignHCenter);

    // ── Styles list (read-only) ──
    m_table = new QTableWidget(0, 4, this);
    m_table->setObjectName(QStringLiteral("sizeStylesTable"));
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    // Compact fixed columns — Name kept small (layer names are short) — and Outline
    // stretchable: it absorbs the dock's free width, and the trace preview grows with it.
    m_table->horizontalHeader()->setSectionResizeMode(kSize, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kName, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kColor, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kOutline, QHeaderView::Stretch);
    m_table->setColumnWidth(kSize, 48);
    m_table->setColumnWidth(kName, 120);
    m_table->setColumnWidth(kColor, 90); // header "Couleur" must not be elided

    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);

    m_table->setItemDelegateForColumn(kColor, new TraceDelegate(m_table));
    m_table->setItemDelegateForColumn(kOutline, new TraceDelegate(m_table));
    m_table->setHorizontalHeaderLabels({tr("Size"), tr("Name"), tr("Color"), tr("Outline")});

    rootLayout->addWidget(m_table, 1);

    UpdateEditButtonIcon();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeStyles::UpdateEditButtonIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    // Normal: current-theme variant. Hover: always the dark variant — the hovered
    // button background is an accent, so a light glyph keeps the contrast (light
    // theme hovers follow the same rule as the "Add a size range" button).
    m_editNormalIcon = MakeCellIcon(QStringLiteral("gear-six"), dark);
    m_editHoverIcon = MakeCellIcon(QStringLiteral("gear-six"), true);
    m_editButton->setIcon(m_editNormalIcon);
}

//---------------------------------------------------------------------------------------------------------------------
bool OWidgetSizeStyles::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editButton)
    {
        if (event->type() == QEvent::Enter)
        {
            m_editButton->setIcon(m_editHoverIcon);
        }
        else if (event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            m_editButton->setIcon(m_editNormalIcon);
        }
    }
    return QWidget::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeStyles::SetSizeStylesContext(const QString &canonicalPath, const QString &axisName,
                                             const QStringList &sizeValues, bool inLibrary)
{
    m_canonicalPath = canonicalPath;
    m_axisName = axisName;
    m_values = sizeValues;
    m_inLibrary = inLibrary;
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeStyles::Populate()
{
    const bool empty = m_values.isEmpty();
    m_table->setRowCount(0);

    // Row actions need the styles store context: an in-library table with a size axis.
    m_editButton->setEnabled(m_inLibrary && !empty && !m_axisName.isEmpty());
    m_editButton->setToolTip(m_inLibrary ? QString()
                                         : tr("Size styles need a measurements table from the library"));

    if (empty || m_axisName.isEmpty())
    {
        return;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const QHash<QString, OGradedSizeStyle> styles =
        OGradedSizeStyles::StylesForAxis(settings, m_canonicalPath, m_axisName);

    m_table->setRowCount(static_cast<int>(m_values.size()));
    for (int i = 0; i < m_values.size(); ++i)
    {
        const QString &sizeValue = m_values.at(i);
        const OGradedSizeStyle style = styles.value(sizeValue); // default when unstyled

        // Size value (always shown — even when the row has no style).
        auto *sizeItem = new QTableWidgetItem(sizeValue);
        sizeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        sizeItem->setFlags(Qt::ItemIsEnabled); // read-only list
        m_table->setItem(i, kSize, sizeItem);

        // Layer-name override (column Name) — empty when none.
        auto *nameItem = new QTableWidgetItem(style.layerName);
        nameItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        nameItem->setFlags(Qt::ItemIsEnabled); // read-only list
        m_table->setItem(i, kName, nameItem);

        // Color + outline are rendered by the delegate from these roles (never as text).
        const QString colorHex = style.HasColor() ? style.color.name() : QString();
        QVariantList dashData;
        dashData.reserve(static_cast<int>(style.dashes.size()));
        for (qreal value : style.dashes)
        {
            dashData.append(value);
        }

        auto *colorItem = new QTableWidgetItem();
        colorItem->setFlags(Qt::ItemIsEnabled); // read-only list
        colorItem->setData(kColorRole, colorHex);
        colorItem->setData(kDashRole, dashData);
        colorItem->setToolTip(colorHex);
        m_table->setItem(i, kColor, colorItem);

        // The Outline cell carries the same style data (same delegate draws both).
        auto *outlineItem = new QTableWidgetItem();
        outlineItem->setFlags(Qt::ItemIsEnabled); // read-only list
        outlineItem->setData(kColorRole, colorHex);
        outlineItem->setData(kDashRole, dashData);
        outlineItem->setToolTip(colorHex);
        m_table->setItem(i, kOutline, outlineItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeStyles::RetranslateUI()
{
    m_editButton->setText(tr("Edit size styles"));
    m_showInCanvas->setText(tr("Show in canvas"));
    m_table->setHorizontalHeaderLabels({tr("Size"), tr("Name"), tr("Color"), tr("Outline")});
    UpdateEditButtonIcon();
    Populate();
}
