/************************************************************************
 **
 **  @file   DrawToolBar.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/08/2026
 **
 **  @brief  Responsive left dock toolbar.
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
#include "DrawToolBar.h"

#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "def.h"
#include "../vmisc/vmodifierkey.h"
#include "vtheme.h"

namespace
{
constexpr int kButtonSize = 32;
constexpr int kSpacing    = 2;
constexpr int kMargins    = 8;
constexpr int kGroupGap   = 6;
constexpr int kHeaderH    = 22;
constexpr int kRowH       = kButtonSize + kSpacing;
constexpr int kFixedWidth = 50;
constexpr int kCaretH     = 16;

// Mode bitmask for group visibility filtering
constexpr int kModeDraw   = 1 << 0; // Draw::Calculation
constexpr int kModeDetail = 1 << 1; // Draw::Modeling
constexpr int kModeLayout = 1 << 2; // Draw::Layout
constexpr int kModeGraded = 1 << 3; // Draw::Graded

int ModeToMask(Draw mode)
{
    switch (mode)
    {
        case Draw::Calculation: return kModeDraw;
        case Draw::Modeling:    return kModeDetail;
        case Draw::Layout:      return kModeLayout;
        case Draw::Graded:      return kModeGraded;
    }
    return 0;
}

auto IsDarkTheme() -> bool
{
    return VTheme::ColorSheme() == VColorSheme::Dark;
}

auto FormatTooltip(const QString &toolName, const QAction *action) -> QString
{
    if (action == nullptr || action->shortcuts().isEmpty())
    {
        return toolName;
    }
    const auto shortcut = VModifierKey::Sequence(action->shortcuts().constFirst()).toLower();
    return QStringLiteral("%1 (%2)").arg(toolName, shortcut);
}
} // anonymous namespace

// ── DrawToolBar ───────────────────────────────────────────────────────────────

DrawToolBar::DrawToolBar(QWidget *parent)
  : QDockWidget(parent)
{
    setObjectName(QStringLiteral("drawToolBar"));
    setWindowTitle(tr("Tools"));
    setFeatures(QDockWidget::NoDockWidgetFeatures);
    setTitleBarWidget(new QWidget(this));

    // Allow the dock to shrink to any width — content must not constrain it.
    // Ignored = Qt ignores sizeHint/minimumSizeHint, the dock can be any size.
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // ── set up widget once, never replaced ──────────────────────────────────
    m_root = new QWidget();
    m_root->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_root->setMinimumSize(0, 0);
    m_rootLayout = new QVBoxLayout(m_root);
    m_rootLayout->setContentsMargins(kMargins / 2, kMargins / 2, kMargins / 2, kMargins / 2);
    m_rootLayout->setSpacing(0);
    setWidget(m_root);

    InitializeGroups();
    setMinimumWidth(ComputeMinWidth());
    BuildLayout();
    resize(ComputeMinWidth() + 4, 100);
}

//---------------------------------------------------------------------------------------------------------------------
auto DrawToolBar::minimumSizeHint() const -> QSize
{
    // Always allow the dock to be shrunk, even if the inner grid layouts
    // think they need more space. The layout will adapt in resizeEvent.
    return QSize(0, 0);
}

DrawToolBar::~DrawToolBar() = default;

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::SetActions(const QHash<QString, QAction *> &actions)
{
    for (auto &group : m_groups)
    {
        for (auto &tool : group.tools)
        {
            if (actions.contains(tool.id))
            {
                tool.action = actions.value(tool.id);
            }
        }
    }

    // Update button tooltips now that actions are set
    for (auto &group : m_groups)
    {
        for (const auto &tool : group.tools)
        {
            if (m_buttonMap.contains(tool.id))
            {
                m_buttonMap[tool.id]->setToolTip(FormatTooltip(tool.name, tool.action));
            }
        }
    }

    BuildLayout();
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::SetMode(const Draw mode)
{
    if (m_currentMode == mode)
    {
        return;
    }

    // Read saved width BEFORE changing constraints — setMinimumWidth triggers
    // a resize event that would overwrite the saved value with the stale width.
    QSettings s;
    const int savedWidth = s.value(QStringLiteral("toolbar/drawWidth"), -1).toInt();

    m_settingMode = true;
    m_currentMode = mode;

    if (mode == Draw::Calculation)
    {
        setMinimumWidth(ComputeMinWidth());
        setMaximumWidth(QWIDGETSIZE_MAX);
    }
    else
    {
        setFixedWidth(kFixedWidth);
    }

    BuildLayout();

    // Now restore the saved width — after the layout is rebuilt and constraints are set.
    if (mode == Draw::Calculation && savedWidth > 0)
    {
        if (auto *mw = qobject_cast<QMainWindow *>(window()))
        {
            mw->resizeDocks({this}, {savedWidth}, Qt::Horizontal);
        }
    }

    m_settingMode = false;
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::ReloadIcons()
{
    const bool dark = IsDarkTheme();
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

    for (auto it = m_buttonMap.cbegin(); it != m_buttonMap.cend(); ++it)
    {
        // Export tool uses a theme-independent PNG — skip it
        if (it.key() == QStringLiteral("detail_export"))
        {
            continue;
        }
        const auto path = QStringLiteral(":/tools/%1/%2.svg").arg(theme, it.key());
        it.value()->setIcon(QIcon(path));
    }

    for (auto *caret : m_caretButtons)
    {
        const auto path = QStringLiteral(":/icons/%1/caret-down_%2.svg").arg(theme, theme);
        caret->setIcon(QIcon(path));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::RestoreSavedWidth()
{
    if (m_currentMode != Draw::Calculation)
    {
        return;
    }
    QSettings s;
    const int savedWidth = s.value(QStringLiteral("toolbar/drawWidth"), -1).toInt();
    if (savedWidth <= 0)
    {
        return;
    }

    // QDockWidget::resize() is ignored when docked —QMainWindow controls dock
    // sizes via resizeDocks(). Use window() to reach the QMainWindow because
    // parentWidget() returns the internal dock container, not the main window.
    QMetaObject::invokeMethod(
        this,
        [this, savedWidth]()
        {
            if (auto *mw = qobject_cast<QMainWindow *>(window()))
            {
                mw->resizeDocks({this}, {savedWidth}, Qt::Horizontal);
            }
        },
        Qt::QueuedConnection);
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::UpdateBackground()
{
    // On macOS, QDockWidget QSS background-color is not reliably re-applied
    // after a theme switch even with a custom title bar. Force the content
    // widget background explicitly — matches the Ostara Old approach.
    // We must use the QSS DockBody token, NOT QPalette::Window, because they
    // diverge in light mode (DockBody=#F4F4F4, Window=#F9F9F9).
    const bool dark = IsDarkTheme();
    const QString bg = dark ? QStringLiteral("#232323") : QStringLiteral("#F4F4F4");
    m_root->setStyleSheet(QStringLiteral("background: %1;").arg(bg));
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);

    // Save geometry for the next session — only user-initiated resizes in Draw mode.
    // Skip during SetMode transitions (constraint changes trigger intermediate resizes
    // with stale widths that would corrupt the saved value).
    if (!m_settingMode && m_currentMode == Draw::Calculation)
    {
        QSettings s;
        s.setValue(QStringLiteral("toolbar/drawWidth"), width());
    }

    if (m_building || m_currentMode != Draw::Calculation)
    {
        return;
    }

    const int newCols = ComputeColumns();
    const int newH = event->size().height();

    if (newCols != m_lastColCount || qAbs(newH - m_lastHeight) > 10)
    {
        m_lastColCount = newCols;
        m_lastHeight = newH;
        BuildLayout();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::StyleChange)
    {
        ReloadIcons();
        UpdateBackground();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        RetranslateGroups();
    }
    QDockWidget::changeEvent(event);
}

// ── Private ───────────────────────────────────────────────────────────────────

int DrawToolBar::ComputeColumns() const
{
    const int avail = width() - kMargins;
    return qMax(1, avail / (kButtonSize + kSpacing));
}

//---------------------------------------------------------------------------------------------------------------------
int DrawToolBar::ComputeMinWidth() const
{
    // Use the same font as the group headers: Inter 11px.
    // QApplication::font() = Inter (set by VTheme::InitFonts).
    QFont f = QApplication::font();
    f.setPixelSize(11);
    const QFontMetrics fm(f);

    int maxW = 0;
    for (const auto &group : m_groups)
    {
        // Measure each header with its actual label text
        maxW = qMax(maxW, fm.horizontalAdvance(group.name));
    }
    // Margins + safety for header padding and style variations
    return maxW + kMargins + 14;
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::InitializeGroups()
{
    const bool dark = IsDarkTheme();

    auto makeBtn = [&](const QString &id) -> QToolButton * {
        // Idempotent: on a re-translate pass the buttons already exist — reuse them.
        if (auto *existing = m_buttonMap.value(id, nullptr))
        {
            return existing;
        }
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));

        const auto iconPath =
            QStringLiteral(":/tools/%1/%2.svg").arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), id);
        btn->setIcon(QIcon(iconPath));

        m_buttonMap.insert(id, btn);
        return btn;
    };

    // clang-format off
    m_groups = {
        {tr("Selection"), {
            {QStringLiteral("arrow_cursor"),            tr("Select"), nullptr, makeBtn("arrow_cursor")},
        }, kModeDraw | kModeDetail | kModeLayout},
        {tr("Points"), {
            {QStringLiteral("segment"),                 tr("Point at distance and angle"),                nullptr, makeBtn("segment")},
            {QStringLiteral("along_line"),              tr("Point at distance along line"),               nullptr, makeBtn("along_line")},
            {QStringLiteral("midpoint"),                tr("Midpoint between two points"),                nullptr, makeBtn("midpoint")},
            {QStringLiteral("normal"),                  tr("Point along perpendicular"),                  nullptr, makeBtn("normal")},
            {QStringLiteral("bisector"),                tr("Point along bisector"),                       nullptr, makeBtn("bisector")},
            {QStringLiteral("height"),                  tr("Perpendicular point along line"),             nullptr, makeBtn("height")},
            {QStringLiteral("point_of_intersection"),   tr("Point from X and Y of two other points"),    nullptr, makeBtn("point_of_intersection")},
            {QStringLiteral("intersect"),               tr("Point at line intersection"),                 nullptr, makeBtn("intersect")},
            {QStringLiteral("shoulder"),                tr("Special point on shoulder"),                  nullptr, makeBtn("shoulder")},
            {QStringLiteral("triangle"),                tr("Triangle tool"),                              nullptr, makeBtn("triangle")},
            {QStringLiteral("line_intersect_axis"),     tr("Point intersect line and axis"),              nullptr, makeBtn("line_intersect_axis")},
        }, kModeDraw},
        {tr("Line"), {
            {QStringLiteral("line"), tr("Line between points"), nullptr, makeBtn("line")},
        }, kModeDraw},
        {tr("Curves"), {
            {QStringLiteral("cubic_bezier"),             tr("Curve tool which uses point as control handle"),    nullptr, makeBtn("cubic_bezier")},
            {QStringLiteral("cubic_bezier_path"),        tr("Curved path tool which uses point as control handle"), nullptr, makeBtn("cubic_bezier_path")},
            {QStringLiteral("curve_intersect_axis"),     tr("Point intersect curve and axis"),                  nullptr, makeBtn("curve_intersect_axis")},
            {QStringLiteral("intersection_curves"),      tr("Point intersection curves"),                       nullptr, makeBtn("intersection_curves")},
            {QStringLiteral("parallel_curve"),           tr("Parallel curve tool"),                             nullptr, makeBtn("parallel_curve")},
            {QStringLiteral("graduated_curve_offseting"), tr("Graduated curve tool"),                           nullptr, makeBtn("graduated_curve_offseting")},
        }, kModeDraw},
        {tr("Arcs"), {
            {QStringLiteral("arc"),                           tr("Arc"),                                      nullptr, makeBtn("arc")},
            {QStringLiteral("arc_with_length"),               tr("Arc with given length"),                    nullptr, makeBtn("arc_with_length")},
            {QStringLiteral("el_arc"),                        tr("Elliptical Arc"),                           nullptr, makeBtn("el_arc")},
            {QStringLiteral("el_arc_with_length"),            tr("Elliptical arc with given length"),         nullptr, makeBtn("el_arc_with_length")},
            {QStringLiteral("arc_cut"),                       tr("Segment an arc"),                           nullptr, makeBtn("arc_cut")},
            {QStringLiteral("arc_start"),                     tr("Arc start point"),                          nullptr, makeBtn("arc_start")},
            {QStringLiteral("arc_end"),                       tr("Arc end point"),                            nullptr, makeBtn("arc_end")},
            {QStringLiteral("point_of_intersection_arcs"),    tr("Point of intersection arcs"),               nullptr, makeBtn("point_of_intersection_arcs")},
            {QStringLiteral("arc_intersect_axis"),            tr("Point of intersection arc and axis"),       nullptr, makeBtn("arc_intersect_axis")},
            {QStringLiteral("point_of_intersection_circles"), tr("Point of intersection circles"),            nullptr, makeBtn("point_of_intersection_circles")},
            {QStringLiteral("point_from_arc_and_tangent"),    tr("Point from arc and tangent"),               nullptr, makeBtn("point_from_arc_and_tangent")},
            {QStringLiteral("point_from_circle_and_tangent"), tr("Point from circle and tangent"),            nullptr, makeBtn("point_from_circle_and_tangent")},
            {QStringLiteral("point_of_contact"),              tr("Point of intersection circle and segment"), nullptr, makeBtn("point_of_contact")},
        }, kModeDraw},
        {tr("Operations"), {
            {QStringLiteral("group_plus"),               tr("Layer"),                  nullptr, makeBtn("group_plus")},
            {QStringLiteral("attach"),                   tr("Group"),                  nullptr, makeBtn("attach")},
            {QStringLiteral("flipping_axis"),            tr("Flipping objects by axis"),          nullptr, makeBtn("flipping_axis")},
            {QStringLiteral("flipping_line"),            tr("Flipping objects by line"),          nullptr, makeBtn("flipping_line")},
            {QStringLiteral("rotation"),                 tr("Rotate objects"),                    nullptr, makeBtn("rotation")},
            {QStringLiteral("move"),                     tr("Move objects"),                      nullptr, makeBtn("move")},
            {QStringLiteral("true_darts"),               tr("True darts"),                        nullptr, makeBtn("true_darts")},
        }, kModeDraw},
        {tr("Pieces"), {
            {QStringLiteral("new_detail"),               tr("Workpiece tool"),                   nullptr, makeBtn("new_detail")},
            {QStringLiteral("path"),                     tr("Internal path tool"),                nullptr, makeBtn("path")},
            {QStringLiteral("insert_node"),              tr("Insert node tool"),                  nullptr, makeBtn("insert_node")},
            {QStringLiteral("place_label"),              tr("Internal mark"),                      nullptr, makeBtn("place_label")},
        }, kModeDraw},
        {tr("Pieces"), {
            {QStringLiteral("union"),                    tr("Union tool"),                        nullptr, makeBtn("union")},
            {QStringLiteral("duplicate_detail"),         tr("Duplicate detail tool"),             nullptr, makeBtn("duplicate_detail")},
        }, kModeDetail},
        {tr("Pieces"), {
            {QStringLiteral("detail_export"),            tr("Export"),                            nullptr, nullptr},
        }, kModeDetail},
        {tr("New"), {
            {QStringLiteral("curve"),                    tr("Curve"),                                            nullptr, makeBtn("curve")},
            {QStringLiteral("curve_cut_point"),          tr("Segment a curve"),                                 nullptr, makeBtn("curve_cut_point")},
            {QStringLiteral("armhole_curve"),            tr("Armhole curve passing through 4 points"),          nullptr, makeBtn("armhole_curve")},
        }, kModeDraw},
    };
    // clang-format on

    // The export tool uses a PNG icon (no SVG theme variants exist yet).
    // Create its button manually — don't go through makeBtn which would try
    // to load a non-existent SVG and spam the console.
    if (!m_buttonMap.contains(QStringLiteral("detail_export")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(QIcon(QStringLiteral(":/icon/32x32/export_to_picture_document.png")));
        m_buttonMap.insert(QStringLiteral("detail_export"), btn);
    }
    // Update the button pointer in the ToolItem
    for (auto &group : m_groups)
    {
        for (auto &tool : group.tools)
        {
            if (tool.id == QStringLiteral("detail_export"))
            {
                tool.button = m_buttonMap.value(QStringLiteral("detail_export"));
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::RetranslateGroups()
{
    InitializeGroups();          // re-evaluates tr() names, reuses existing buttons
    setWindowTitle(tr("Tools"));
    BuildLayout();               // re-renders group headers + tooltips with the new names
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::BuildLayout()
{
    if (m_building)
    {
        return;
    }
    m_building = true;

    const int cols = ComputeColumns();
    const int availH = qMax(100, height() - kMargins);

    // ── 1. Filter groups by mode ─────────────────────────────────────────
    QVector<int> visibleIdx;
    int totalToolCount = 0;
    const int currentMask = ModeToMask(m_currentMode);

    for (int g = 0; g < static_cast<int>(m_groups.size()); ++g)
    {
        const auto &group = m_groups[g];

        // Check if the group is visible in the current mode
        if ((group.modeMask & currentMask) == 0)
        {
            // Hide all buttons in skipped groups
            for (const auto &tool : group.tools)
            {
                if (m_buttonMap.contains(tool.id))
                {
                    m_buttonMap[tool.id]->setVisible(false);
                }
            }
            continue;
        }

        visibleIdx.append(g);
        totalToolCount += static_cast<int>(group.tools.size());
    }

    // In non-Draw modes, the toolbar is fixed-width — no headers, no overflow.
    const bool compactMode = (m_currentMode != Draw::Calculation);

    const int numVisible = static_cast<int>(visibleIdx.size());

    // ── 2. Compute row budget ────────────────────────────────────────────
    const int totalHeadersH = numVisible * (kHeaderH + kGroupGap);
    const int toolH = qMax(numVisible * kRowH, availH - totalHeadersH);
    const int toolRows = toolH / kRowH;

    // ── 3. Allocate: 1 row minimum per group, proportional for the rest ──
    QVector<int> allocated(numVisible, 1); // minimum 1 row per group

    int extraRows = toolRows - numVisible;
    if (extraRows > 0 && totalToolCount > 0)
    {
        for (int i = 0; i < extraRows; ++i)
        {
            // Find the group that would benefit most from an extra row
            int bestIdx = -1;
            double bestRatio = 0.0;
            for (int j = 0; j < numVisible; ++j)
            {
                const int g = visibleIdx[j];
                const int naturalRows =
                    (static_cast<int>(m_groups[g].tools.size()) + cols - 1) / qMax(1, cols);
                if (allocated[j] >= naturalRows)
                {
                    continue; // already fits, no benefit
                }
                const double ratio = static_cast<double>(static_cast<int>(m_groups[g].tools.size())) /
                                     qMax(1, allocated[j] * cols);
                if (ratio > bestRatio)
                {
                    bestRatio = ratio;
                    bestIdx = j;
                }
            }
            if (bestIdx < 0)
            {
                break;
            }
            allocated[bestIdx]++;
        }
    }

    // ── 4. Clean up old widgets (keep tool buttons, delete the rest) ──────
    m_caretButtons.clear();
    // Remove all layout items from rootLayout
    QLayoutItem *child;
    while ((child = m_rootLayout->takeAt(0)) != nullptr)
    {
        if (child->widget() != nullptr)
        {
            // Only delete widgets that are NOT our persistent tool buttons
            if (child->widget()->objectName() != QStringLiteral("drawToolBtn"))
            {
                child->widget()->deleteLater();
            }
        }
        delete child;
    }

    // Delete any remaining header/caret children not caught by the layout cleanup
    const auto children = m_root->children();
    for (auto *c : children)
    {
        if (auto *w = qobject_cast<QWidget *>(c))
        {
            if (w->objectName() != QStringLiteral("drawToolBtn"))
            {
                w->deleteLater();
            }
        }
    }

    // ── 5. Rebuild layout ────────────────────────────────────────────────
    for (int vi = 0; vi < numVisible; ++vi)
    {
        const int g = visibleIdx[vi];
        const auto &group = m_groups[g];
        const int visRows = allocated[vi];
        const int visTools = visRows * cols;
        const int totalTools = static_cast<int>(group.tools.size());
        const bool hasCaret = !compactMode && (totalTools > visTools);
        const bool dark = IsDarkTheme();
        const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

        if (group.tools.isEmpty())
        {
            if (!compactMode)
            {
                auto *header = new QLabel(group.name, m_root);
                header->setObjectName(QStringLiteral("toolGroupHeader"));
                header->setAlignment(Qt::AlignCenter);
                header->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(
                    IsDarkTheme() ? QStringLiteral("#C2C2C2") : QStringLiteral("#A0A0A8")));
                m_rootLayout->addWidget(header);
                m_rootLayout->addSpacing(kGroupGap);
            }
            continue;
        }

        if (!compactMode)
        {
            auto *header = new QLabel(group.name, m_root);
            header->setObjectName(QStringLiteral("toolGroupHeader"));
            header->setAlignment(Qt::AlignCenter);
            header->setStyleSheet(
                QStringLiteral("color: %1; font-size: 11px; padding-top: 4px; padding-bottom: 2px;")
                    .arg(IsDarkTheme() ? QStringLiteral("#C2C2C2") : QStringLiteral("#A0A0A8")));
            m_rootLayout->addWidget(header);
        }

        auto *grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(kSpacing);
        grid->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        QMenu *flyoutMenu = nullptr;
        if (hasCaret)
        {
            flyoutMenu = new QMenu(m_root);
        }

        int row = 0;
        int col = 0;

        for (int i = 0; i < totalTools; ++i)
        {
            const auto &tool = group.tools.at(i);

            if (m_buttonMap.contains(tool.id))
            {
                auto *btn = m_buttonMap[tool.id];
                btn->setVisible(i < visTools);

                // Remove from any previous layout
                if (auto *oldLayout = btn->parentWidget()->layout())
                {
                    oldLayout->removeWidget(btn);
                }
                btn->setParent(m_root);

                grid->addWidget(btn, row, col);

                if (tool.action != nullptr)
                {
                    connect(btn, &QToolButton::clicked, tool.action, &QAction::trigger, Qt::UniqueConnection);
                    connect(tool.action, &QAction::toggled, btn, &QToolButton::setChecked, Qt::UniqueConnection);
                    btn->setChecked(tool.action->isChecked());
                    btn->setToolTip(FormatTooltip(tool.name, tool.action));

                    // Only overflow (hidden) tools go in the flyout menu.
                    // Set the themed SVG icon on the action so the flyout
                    // matches the toolbar (old Valentina action icons don't
                    // adapt to light/dark). Skip tools that use PNG icons.
                    if (flyoutMenu != nullptr && i >= visTools)
                    {
                        if (tool.id != QStringLiteral("detail_export"))
                        {
                            const auto iconPath = QStringLiteral(":/tools/%1/%2.svg").arg(theme, tool.id);
                            tool.action->setIcon(QIcon(iconPath));
                        }
                        flyoutMenu->addAction(tool.action);
                    }
                }
            }

            if (++col >= cols)
            {
                col = 0;
                ++row;
            }
        }

        // Caret — centered across all columns
        if (hasCaret && flyoutMenu != nullptr)
        {
            const bool dark = IsDarkTheme();
            const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
            const auto caretPath = QStringLiteral(":/icons/%1/caret-down_%2.svg").arg(theme, theme);

            auto *caretBtn = new QToolButton(m_root);
            caretBtn->setObjectName(QStringLiteral("drawToolCaret"));
            caretBtn->setAutoRaise(true);
            caretBtn->setFixedSize(kButtonSize, kCaretH);
            caretBtn->setIconSize(QSize(kCaretH, kCaretH));
            caretBtn->setIcon(QIcon(caretPath));
            caretBtn->setToolTip(tr("%1 more tools").arg(totalTools - visTools));
            caretBtn->setPopupMode(QToolButton::InstantPopup);
            caretBtn->setMenu(flyoutMenu);

            m_caretButtons.append(caretBtn);

            // Place the caret in the next row, centered across all columns
            const int caretRow = row + 1;
            auto *caretLayout = new QHBoxLayout();
            caretLayout->setContentsMargins(0, 0, 0, 0);
            caretLayout->setAlignment(Qt::AlignCenter);
            caretLayout->addWidget(caretBtn);
            grid->addLayout(caretLayout, caretRow, 0, 1, cols);
        }

        m_rootLayout->addLayout(grid);
        m_rootLayout->addSpacing(kGroupGap);
    }

    m_rootLayout->addStretch();
    m_building = false;
}
