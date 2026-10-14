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

// Mode bitmask for group visibility filtering
constexpr int kModeDraw   = 1 << 0; // Draw::Calculation
constexpr int kModeDetail = 1 << 1; // Draw::Modeling
constexpr int kModeLayout = 1 << 2; // Draw::Layout
constexpr int kModeGraded = 1 << 3; // Draw::Graded
constexpr int kModeExport = 1 << 4; // Draw::Export

int ModeToMask(Draw mode)
{
    switch (mode)
    {
        case Draw::Calculation: return kModeDraw;
        case Draw::Modeling:    return kModeDetail;
        case Draw::Layout:      return kModeLayout;
        case Draw::Graded:      return kModeGraded;
        case Draw::Export:      return kModeExport;
    }
    return 0;
}

auto IsDarkTheme() -> bool
{
    return VTheme::ColorSheme() == VColorSheme::Dark;
}

/// Whether @p id is one of the export buttons: they read their icon from :/icons/ or from a PNG, never from
/// :/tools/ — there is no <id>.svg for them.
auto IsExportIconTool(const QString &id) -> bool
{
    return id == QStringLiteral("detail_export") || id == QStringLiteral("export_templates")
           || id == QStringLiteral("export_image");
}

/// The icons the tools that wait for artwork of their own borrow, from the tool they come from: looking for a
/// <id>.svg that does not exist would blank them on every theme switch. Read by IsAppIconTool, and kept in step
/// with the install blocks by hand — a borrowed icon has to be added here too.
auto BorrowedToolIcons() -> const QHash<QString, QString> &
{
    static const QHash<QString, QString> borrowed{{QStringLiteral("close_dart"), QStringLiteral("rotation.png")},
                                                  {QStringLiteral("dart_cap"), QStringLiteral("true_darts.png")},
                                                  {QStringLiteral("rectangle"), QStringLiteral("shoulder.png")},
                                                  {QStringLiteral("transfert"), QStringLiteral("move.png")},
                                                  {QStringLiteral("spiral"), QStringLiteral("parallel_curve.png")},
                                                  {QStringLiteral("spread"), QStringLiteral("arc_with_length.png")},
                                                  {QStringLiteral("move_batch"), QStringLiteral("el_arc.png")},
                                                  {QStringLiteral("pleat"), QStringLiteral("triangle.png")},
                                                  {QStringLiteral("pleat_mid"), QStringLiteral("height.png")},
                                                  {QStringLiteral("flounce_spiral"), QStringLiteral("point_of_intersection_circles.png")}};
    return borrowed;
}

/// Whether @p id carries an icon the APPLICATION sets, rather than one of the tool icons of
/// :/tools/ — a PNG that has no theme, or an SVG of :/icons/ that has two.
///
/// Three places ask this same question and must not answer it differently: makeBtn would look for a file that
/// does not exist, ReloadIcons must not overwrite what was set, and the flyout must not either. The two helpers
/// below are the agreed list — but the install blocks and the ReloadIcons branches are written by hand and do
/// not read them, so an id added there has to be added here too, or the button keeps a blank icon and nothing
/// says so.
auto IsAppIconTool(const QString &id) -> bool
{
    return IsExportIconTool(id) || BorrowedToolIcons().contains(id);
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
    const bool dark = IsDarkTheme();
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

    for (auto it = m_buttonMap.cbegin(); it != m_buttonMap.cend(); ++it)
    {
        // Export tool uses a theme-independent PNG — skip it
        if (it.key() == QStringLiteral("detail_export"))
        {
            continue;
        }
        // Export templates uses an application-set icon, not a tool icon
        if (it.key() == QStringLiteral("export_templates"))
        {
            it.value()->setIcon(
                QIcon(QStringLiteral(":/icons/%1/grid-four-fill_%1.svg").arg(theme)));
            continue;
        }
        // Export as image, likewise
        if (it.key() == QStringLiteral("export_image"))
        {
            it.value()->setIcon(
                QIcon(QStringLiteral(":/icons/%1/file-image-fill_%1.svg").arg(theme)));
            continue;
        }
        // Tools that wait for artwork of their own borrow the icon of the tool they come from: looking for a
        // <id>.svg that does not exist would blank them on every theme switch.
        const QHash<QString, QString> &borrowed = BorrowedToolIcons();
        if (const auto icon = borrowed.constFind(it.key()); icon != borrowed.cend())
        {
            it.value()->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), icon.value()));
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
        }, kModeDraw | kModeDetail | kModeLayout | kModeExport},
        {tr("Export templates"), {
            {QStringLiteral("export_templates"),        tr("Export templates"), nullptr, nullptr},
        }, kModeExport},
        // A group of its own for one tool, and that is structural rather than tidy: outside
        // Draw::Calculation there is no caret (hasCaret is false in a compact mode), so a second
        // tool of a group that the row budget cannot fit is hidden with NOWHERE to find it —
        // setVisible(i < visTools), and no flyout to hold it. A group of one cannot overflow.
        {tr("Export image"), {
            {QStringLiteral("export_image"),            tr("Export as image..."), nullptr, nullptr},
        }, kModeExport},
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
            // No close_dart.svg exists yet, so this button is built by hand further down and borrows the icon
            // of the rotation it comes from. The tool waits here until it has artwork of its own.
            {QStringLiteral("close_dart"),               tr("Close dart tool"),                                 nullptr, nullptr},
            // Same as close_dart: no dart_cap.svg exists yet, so the button is built by hand further down
            // and borrows the icon of true darts.
            {QStringLiteral("dart_cap"),                 tr("Dart cap tool"),                                   nullptr, nullptr},
            // Same again: the rectangle waits for artwork of its own and borrows the icon of the shoulder
            // point, which is where its dialog takes its cursor from too.
            {QStringLiteral("rectangle"),                tr("Rectangle tool"),                                  nullptr, nullptr},
            // Same again: no transfert.svg yet, so the button is built by hand below and borrows the icon of
            // the move it translates — the one its action uses too.
            {QStringLiteral("transfert"),                tr("Transfer tool"),                                   nullptr, nullptr},
            // Same again: the spiral waits for artwork of its own and borrows the icon of the parallel curve,
            // which is where its action takes it from too.
            {QStringLiteral("spiral"),                   tr("Spiral tool"),                                     nullptr, nullptr},
            // Same again: the slash-and-spread borrows the icon of the arc of given length.
            {QStringLiteral("spread"),                   tr("Slash and spread tool"),                           nullptr, nullptr},
            // Same again: the move-by-batch borrows the icon of the elliptical arc.
            {QStringLiteral("move_batch"),               tr("Move by batch tool"),                              nullptr, nullptr},
            // Same again: the pleat borrows the icon of the triangle.
            {QStringLiteral("pleat"),                    tr("Pleat tool"),                                      nullptr, nullptr},
            // Same again: the middle pleat borrows the icon of the perpendicular point on a segment.
            {QStringLiteral("pleat_mid"),                tr("Middle pleat tool"),                               nullptr, nullptr},
            // Same again: the flounce spiral waits for artwork of its own and borrows the icon of the point at
            // the intersection of two circles, which is the construction it draws.
            {QStringLiteral("flounce_spiral"),           tr("Flounce spiral tool"),                             nullptr, nullptr},
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
    // The Export templates button carries an application-set themed icon (:/icons/…), not a
    // tool icon from :/tools/ — created manually for the same reason as the export tool.
    if (!m_buttonMap.contains(QStringLiteral("export_templates")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        // Not checkable: it opens a window, it does not arm a tool.
        btn->setCheckable(false);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(QIcon(QStringLiteral(":/icons/%1/grid-four-fill_%1.svg")
                               .arg(IsDarkTheme() ? QStringLiteral("dark") : QStringLiteral("light"))));
        m_buttonMap.insert(QStringLiteral("export_templates"), btn);
    }
    // The image export button carries an application-set themed icon (:/icons/…), exactly like
    // Export templates above — same reason, same shape.
    if (!m_buttonMap.contains(QStringLiteral("export_image")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        // Not checkable: it opens a dialog, it does not arm a tool.
        btn->setCheckable(false);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(QIcon(QStringLiteral(":/icons/%1/file-image-fill_%1.svg")
                               .arg(IsDarkTheme() ? QStringLiteral("dark") : QStringLiteral("light"))));
        m_buttonMap.insert(QStringLiteral("export_image"), btn);
    }

    // The tool that closes a dart waits for its own artwork: with no close_dart.svg to load — and makeBtn()
    // would complain about it — its button borrows the icon of the rotation it comes from, from the
    // application rather than from :/tools/. Built by hand for the same reason as the export tool.
    if (!m_buttonMap.contains(QStringLiteral("dart_cap")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("true_darts.png")));
        m_buttonMap.insert(QStringLiteral("dart_cap"), btn);
    }

    if (!m_buttonMap.contains(QStringLiteral("close_dart")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("rotation.png")));
        m_buttonMap.insert(QStringLiteral("close_dart"), btn);
    }

    // The rectangle waits for artwork of its own: with no rectangle.svg to load, its button borrows the icon
    // of the shoulder point — the same one its action and its cursor come from. By hand, like the two above.
    if (!m_buttonMap.contains(QStringLiteral("rectangle")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("shoulder.png")));
        m_buttonMap.insert(QStringLiteral("rectangle"), btn);
    }

    // The transfer waits for artwork of its own too, and borrows the icon of the move — same one its action
    // carries. By hand, like the three above.
    if (!m_buttonMap.contains(QStringLiteral("transfert")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("move.png")));
        m_buttonMap.insert(QStringLiteral("transfert"), btn);
    }

    // The spiral waits for artwork of its own too, and borrows the icon of the parallel curve — the same one
    // its action carries. By hand, like the four above.
    if (!m_buttonMap.contains(QStringLiteral("spiral")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("parallel_curve.png")));
        m_buttonMap.insert(QStringLiteral("spiral"), btn);
    }

    // The flounce spiral waits for artwork of its own too, and borrows the icon of the point at the
    // intersection of two circles — the same one its action carries.
    if (!m_buttonMap.contains(QStringLiteral("flounce_spiral")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"),
                                             QStringLiteral("point_of_intersection_circles.png")));
        m_buttonMap.insert(QStringLiteral("flounce_spiral"), btn);
    }

    // The slash-and-spread borrows the icon of the arc of given length — the same one its action carries.
    if (!m_buttonMap.contains(QStringLiteral("spread")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("arc_with_length.png")));
        m_buttonMap.insert(QStringLiteral("spread"), btn);
    }

    // The move-by-batch borrows the icon of the elliptical arc — the same one its action carries.
    if (!m_buttonMap.contains(QStringLiteral("move_batch")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("el_arc.png")));
        m_buttonMap.insert(QStringLiteral("move_batch"), btn);
    }

    // The pleat borrows the icon of the triangle — the same one its action carries.
    if (!m_buttonMap.contains(QStringLiteral("pleat")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("triangle.png")));
        m_buttonMap.insert(QStringLiteral("pleat"), btn);
    }

    // The middle pleat borrows the icon of the perpendicular point on a segment — the same one its action carries.
    if (!m_buttonMap.contains(QStringLiteral("pleat_mid")))
    {
        auto *btn = new QToolButton(m_root);
        btn->setObjectName(QStringLiteral("drawToolBtn"));
        btn->setAutoRaise(true);
        btn->setCheckable(true);
        btn->setFixedSize(kButtonSize, kButtonSize);
        btn->setIconSize(QSize(24, 24));
        btn->setIcon(VTheme::GetIconResource(QStringLiteral("toolicon"), QStringLiteral("height.png")));
        m_buttonMap.insert(QStringLiteral("pleat_mid"), btn);
    }

    // Update the button pointer in the ToolItem
    for (auto &group : m_groups)
    {
        for (auto &tool : group.tools)
        {
            if (IsAppIconTool(tool.id))
            {
                tool.button = m_buttonMap.value(tool.id);
            }
        }
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
        const bool dark = IsDarkTheme();
        const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

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
                        if (!IsAppIconTool(tool.id))
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
