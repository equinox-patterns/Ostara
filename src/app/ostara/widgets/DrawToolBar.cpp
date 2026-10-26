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
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "def.h"
#include "../core/otoolbarcatalogue.h"
#include "../core/otoolbarlayout.h"
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

// The narrowest the palette may be dragged to: one column of icons. The content never constrains the dock — the
// root widget is Ignored with a zero minimum, and minimumSizeHint() is null on purpose (see the constructor).
// A floor computed from the longest group header instead — "Export templates" — used to land just above the
// three-column threshold of ComputeColumns(), so the palette could be widened but never narrowed again.
constexpr int kMinPaletteWidth = kButtonSize + kMargins;

// Mode bitmask for group visibility filtering. The values live in the catalogue, which writes a
// group's mask: a second copy here could drift from it.
constexpr int kModeDraw   = OToolBarCatalogue::ModeDraw;   // Draw::Calculation
constexpr int kModeDetail = OToolBarCatalogue::ModeDetail; // Draw::Modeling
constexpr int kModeGraded = OToolBarCatalogue::ModeGraded; // Draw::Graded
constexpr int kModeExport = OToolBarCatalogue::ModeExport; // Draw::Export

int ModeToMask(Draw mode)
{
    switch (mode)
    {
        case Draw::Calculation: return kModeDraw;
        case Draw::Modeling:    return kModeDetail;
        case Draw::Graded:      return kModeGraded;
        case Draw::Export:      return kModeExport;
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

/// The name of a group, above its icons. It elides its own text because the palette can be dragged down to a
/// single column, where the longest name — "Export templates" — is wider than the dock itself.
///
/// It follows the drag on its own: the layout is rebuilt only when the column count changes, so the elision
/// cannot wait for BuildLayout(). The full name stays readable through the tooltip.
class GroupHeaderLabel : public QLabel
{
public:
    GroupHeaderLabel(const QString &name, QWidget *parent)
      : QLabel(name, parent),
        m_name(name)
    {
        setToolTip(name);
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QLabel::resizeEvent(event);
        // contentsRect(), not rect(): the padding of the style sheet is not room for text.
        const QString shown = fontMetrics().elidedText(m_name, Qt::ElideRight, contentsRect().width());
        // Set it only when it changes — setText() invalidates the geometry, and a resize that always rewrote the
        // text would ask for another resize for nothing.
        if (shown != QLabel::text())
        {
            QLabel::setText(shown);
        }
    }

private:
    QString m_name;
};
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
    // One column is the floor: the palette has to be narrowable back to a single file of icons.
    setMinimumWidth(kMinPaletteWidth);
    BuildLayout();
    // Wider than that by default: the width the group headers need to be read in full.
    resize(ComputeDefaultWidth() + 4, 100);
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
    // The map outlives every rebuild of `m_groups` — that is the whole point of keeping it.
    m_actionMap = actions;
    ApplyStoredActions();

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
/**
 * @brief Re-attach the stored actions to the tools of a freshly rebuilt `m_groups`.
 *
 * `InitializeGroups()` rebuilds `m_groups` from a literal table whose action column holds nothing
 * but `nullptr`, and restores only the buttons. Everything `BuildLayout()` does for a tool lives
 * under `if (tool.action != nullptr)` — the tooltip, the checked state, the click connection and
 * the overflow menu entry. Without this call the first rebuild silently dropped all four: the
 * tooltips kept the language they were written in at startup, and the tools that do not fit in the
 * rows became unreachable, the caret menu having nothing left to list.
 */
void DrawToolBar::ApplyStoredActions()
{
    for (auto &group : m_groups)
    {
        for (auto &tool : group.tools)
        {
            tool.action = m_actionMap.value(tool.id, nullptr);
        }
    }
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
        setMinimumWidth(kMinPaletteWidth);
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
    // Every button takes its icon from the catalogue: the tools that borrow another tool's artwork,
    // and the three the application sets, are decided there once instead of in a branch per id.
    for (auto it = m_buttonMap.cbegin(); it != m_buttonMap.cend(); ++it)
    {
        it.value()->setIcon(OToolBarCatalogue::IconFor(it.key()));
    }

    const QString theme = IsDarkTheme() ? QStringLiteral("dark") : QStringLiteral("light");
    for (auto *caret : m_caretButtons)
    {
        caret->setIcon(QIcon(QStringLiteral(":/icons/%1/caret-down_%2.svg").arg(theme, theme)));
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

    // width()/height() rather than event->size(): a resize event delivered late (a hidden dock that was resized
    // while it was away) carries an older size, while these are the size the layout would actually be built for —
    // the same pair ComputeColumns() and BuildLayout() read.
    const int newCols = ComputeColumns();
    const int newH = height();

    // m_lastColCount and m_lastHeight describe what BuildLayout() last drew — it is their only writer, so that
    // a rebuild asked from anywhere else cannot leave them describing a layout that is no longer on screen.
    if (newCols != m_lastColCount || qAbs(newH - m_lastHeight) > 10)
    {
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
int DrawToolBar::ComputeDefaultWidth() const
{
    // The width the palette opens with when no width has been saved: the longest group name, measured in the
    // font of the headers (Inter 11px, set by VTheme::InitFonts), plus the margins and a little safety. It is
    // deliberately NOT a minimum — the floor is kMinPaletteWidth, or the palette could never be narrowed.
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
    const auto makeBtn = [this](const QString &id) -> QToolButton * {
        // Idempotent: on a rebuild the buttons already exist — reuse them.
        if (auto *existing = m_buttonMap.value(id, nullptr))
        {
            return existing;
        }
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(OToolBarCatalogue::IsCheckable(id));
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(OToolBarCatalogue::IconFor(id));

        m_buttonMap.insert(id, btn);
        return btn;
    };

    const OToolBarLayout::Layout layout = OToolBarLayout::Load();

    m_groups.clear();

    // The Draw mode: what the user arranged in the preferences, or the eight groups Ostara ships
    // with. A tool the user left out of every group is simply not here — and not drawn.
    for (const OToolBarLayout::Group &group : layout.groups)
    {
        ToolGroup built;
        built.name = group.DisplayName();
        built.modeMask = kModeDraw;
        for (const QString &id : group.tools)
        {
            built.tools.append(ToolItem{id, OToolBarCatalogue::ToolName(id), nullptr, makeBtn(id)});
        }
        m_groups.append(built);
    }

    // The other modes are not user-configurable: outside Draw::Calculation there is no caret, so a
    // group the row budget cannot fit would have nowhere to put its overflow.
    for (const OToolBarCatalogue::Group &group : OToolBarCatalogue::OtherGroups())
    {
        ToolGroup built;
        built.name = group.name;
        built.modeMask = group.modeMask;
        for (const QString &id : group.tools)
        {
            built.tools.append(ToolItem{id, OToolBarCatalogue::ToolName(id), nullptr, makeBtn(id)});
        }
        m_groups.append(built);
    }

    // A button whose tool left every group survives in the map — it is a child of the root widget,
    // and an orphan child sits at the top-left corner. Hide it explicitly; BuildLayout() only ever
    // visits the tools that are in a group.
    const QStringList shown = layout.Assigned();
    for (auto it = m_buttonMap.cbegin(); it != m_buttonMap.cend(); ++it)
    {
        it.value()->setVisible(shown.contains(it.key()));
    }

    // And put the actions back: the table above cannot carry them.
    ApplyStoredActions();
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::RetranslateGroups()
{
    InitializeGroups();          // re-evaluates tr() names, reuses existing buttons
    setWindowTitle(tr("Tools"));
    BuildLayout();               // re-renders group headers + tooltips with the new names
}

//---------------------------------------------------------------------------------------------------------------------
void DrawToolBar::ReloadLayout()
{
    InitializeGroups();          // the groups, their names and their tools come from the layout
    BuildLayout();
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

    // What this call is about to draw, and the height its row budget is based on. resizeEvent() compares the
    // columns it would need against these to decide whether a rebuild is worth it, so they describe the LAYOUT
    // ON SCREEN and belong here, not only in resizeEvent(): BuildLayout() is also reached from SetMode(),
    // SetActions() and RetranslateGroups(), where the width is not the one the last resize event saw. Recorded
    // only there, they lied — the palette kept showing a single column at a width that fitted three, until a
    // resize changed the column count and forced the rebuild.
    m_lastColCount = cols;
    m_lastHeight = height();

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

        if (group.tools.isEmpty())
        {
            if (!compactMode)
            {
                auto *header = new GroupHeaderLabel(group.name, m_root);
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
            auto *header = new GroupHeaderLabel(group.name, m_root);
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
                        tool.action->setIcon(OToolBarCatalogue::IconFor(tool.id));
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
