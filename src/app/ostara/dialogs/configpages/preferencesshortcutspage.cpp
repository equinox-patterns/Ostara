/************************************************************************
 **
 **  @file   preferencesshortcutspage.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#include "preferencesshortcutspage.h"

#include "../vmisc/dialogs/vshortcutdialog.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractshortcutmanager.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "vtheme.h"
#include "ui_preferencesshortcutspage.h"

#include <QFont>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>

namespace
{
/// The index of the shortcut in the manager's list, on the first column of an action row.
constexpr int kShortcutIndexRole = Qt::UserRole;
/// What kind of row it is, on the first column of every row. One shortcut can be told from one section only so.
constexpr int kKindRole = Qt::UserRole + 1;

/// @brief The caret of a section: folded points down (its rows are below, one click brings them out), open
/// points up — the pair the table of variables and the formula block already wear.
///
/// The colour follows Icons.md §2: the theme's variant at rest, the dark set the moment the row is under the
/// cursor or chosen — those rows take the accent background, and a #2b2b2b glyph dies on it. `QIcon::Selected`
/// covers the choice; the hover is swapped by hand, because Qt never asks a menu-less item view for `Active`
/// (qcommonstyle.cpp, `CE_ItemViewItem`: only Disabled and Selected are mapped).
auto SectionCaret(bool folded, bool highlighted) -> QIcon
{
    const QString caret = folded ? QStringLiteral("caret-down") : QStringLiteral("caret-up");
    const auto path = [](const QString &theme, const QString &name) -> QString
    { return QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, name); };

    const QString theme = VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                                   : QStringLiteral("light");

    QIcon icon;
    icon.addFile(path(highlighted ? QStringLiteral("dark") : theme, caret), QSize(), QIcon::Normal);
    icon.addFile(path(QStringLiteral("dark"), caret), QSize(), QIcon::Selected);
    icon.addFile(path(QStringLiteral("dark"), caret), QSize(), QIcon::Active);
    return icon;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
PreferencesShortcutsPage::PreferencesShortcutsPage(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::PreferencesShortcutsPage)
{
    ui->setupUi(this);

    // Two columns, each half of the table, and the split stays the reader's: a divider dragged by hand is
    // respected. The split is driven by the viewport and not by this page — see eventFilter().
    QHeaderView *header = ui->shortcutsTable->horizontalHeader();
    header->setStretchLastSection(false); // the .ui asks for it, and it would keep the last column for itself
    header->setSectionResizeMode(QHeaderView::Interactive);
    ui->shortcutsTable->viewport()->installEventFilter(this);
    header->viewport()->installEventFilter(this);

    LoadFoldedSections();

    connect(ui->lineEditSearch, &QLineEdit::textChanged, this, [this]() { FillTable(); });
    connect(ui->shortcutsTable, &QTableWidget::entered, this,
            [this](const QModelIndex &index) { HoverSectionRow(index.isValid() ? index.row() : -1); });
    connect(ui->shortcutsTable, &QTableWidget::cellClicked, this, &PreferencesShortcutsPage::ShortcutCellClicked);
    connect(ui->shortcutsTable, &QTableWidget::cellDoubleClicked, this,
            &PreferencesShortcutsPage::ShortcutCellDoubleClicked);
    connect(ui->pushButtonRestoreDefaults, &QPushButton::clicked, this, [this]() { InitShortcuts(true); });

    InitShortcuts();
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::eventFilter(QObject *watched, QEvent *event) -> bool
{
    QHeaderView *header = ui->shortcutsTable->horizontalHeader();

    if (watched == ui->shortcutsTable->viewport() && event->type() == QEvent::Resize && !m_splittingColumns)
    {
        SplitColumnsEvenly();
    }
    else if (watched == header->viewport() && event->type() == QEvent::MouseButtonPress)
    {
        // A divider is only ever dragged after pressing on the header. This is the one sign that cannot come
        // from our own code, unlike sectionResized(), which our resizeSection() calls emit as well.
        m_userChoseColumnWidths = true;
    }

    return QWidget::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::SplitColumnsEvenly()
{
    QHeaderView *header = ui->shortcutsTable->horizontalHeader();
    const int width = ui->shortcutsTable->viewport()->width();

    if (m_userChoseColumnWidths || width <= 0 || header->count() != 2)
    {
        return;
    }

    m_splittingColumns = true;
    header->resizeSection(0, width / 2);
    header->resizeSection(1, width - width / 2);
    m_splittingColumns = false;
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesShortcutsPage::~PreferencesShortcutsPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::Apply() -> QStringList
{
    if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
    {
        const auto &shortcutsList = manager->GetShortcutsList();
        for (int i = 0; i < m_transientShortcuts.length(); i++)
        {
            const QStringList &current = m_transientShortcuts.value(i);
            const QStringList &defaults = shortcutsList.value(i).defaultShortcuts;

            // Only persist shortcuts that differ from code defaults.
            // When a default changes (e.g. Cmd+W → Cmd+D for DrawMode), the old
            // persisted value must not lock the user to the stale shortcut.
            if (current != defaults)
            {
                VAbstractValApplication::VApp()->ValentinaSettings()->SetActionShortcuts(
                    VAbstractShortcutManager::ShortcutActionToString(shortcutsList.value(i).type), current);
            }
            else
            {
                // Remove any stale persisted value so the code default applies.
                VAbstractValApplication::VApp()->ValentinaSettings()->RemoveActionShortcuts(
                    VAbstractShortcutManager::ShortcutActionToString(shortcutsList.value(i).type));
            }
        }

        manager->UpdateShortcuts();
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        ui->retranslateUi(this);
        // The sections wear translated names too, and the table holds them: the whole table is rebuilt rather
        // than patched, so that a heading cannot keep the previous language.
        FillTable();
    }
    // remember to call base class implementation
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::ShortcutCellClicked(int row, int column)
{
    Q_UNUSED(column)

    if (row < 0 || row >= m_rows.size() || m_rows.at(row).kind != RowKind::Section)
    {
        return;
    }

    ToggleSection(row);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::ShortcutCellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)

    // A section has nothing to edit: opening the shortcut dialog on its row would edit the shortcut of whatever
    // row happens to have that index.
    if (row < 0 || row >= m_rows.size() || m_rows.at(row).kind != RowKind::Action)
    {
        return;
    }

    auto *shortcutDialog = new VShortcutDialog(m_rows.at(row).shortcutIndex, this);
    connect(shortcutDialog, &VShortcutDialog::ShortcutsListChanged, this,
            [this](int index, const QStringList &stringListShortcuts)
            {
                m_transientShortcuts.replace(index, stringListShortcuts);
                UpdateShortcutsTable();
            });
    shortcutDialog->open();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::InitShortcuts(bool defaults)
{
    VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager();
    if (manager == nullptr)
    {
        return;
    }

    manager->UpdateShortcuts();

    m_transientShortcuts.clear();
    m_actions.clear();

    const auto &shortcutsList = manager->GetShortcutsList();
    for (int i = 0; i < shortcutsList.length(); i++)
    {
        const VAbstractShortcutManager::VSShortcut &shortcut = shortcutsList.value(i);

        m_actions.append(shortcut.type);

        // Add shortcut to transient shortcut list
        m_transientShortcuts.append(defaults ? shortcut.defaultShortcuts : shortcut.shortcuts);
    }

    FillTable();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::FillTable()
{
    const QString needle = ui->lineEditSearch->text().trimmed();

    m_rows.clear();

    for (int s = 0; s < static_cast<int>(Section::LAST_ONE_DO_NOT_USE); s++)
    {
        const auto section = static_cast<Section>(s);
        // A section answers for itself: its own name matching shows everything it holds — the search is not
        // about the rows then.
        const bool wholeSection = !needle.isEmpty() && SectionName(section).contains(needle, Qt::CaseInsensitive);

        QVector<Row> content;
        for (int index = 0; index < m_actions.size(); index++)
        {
            const VShortcutAction action = m_actions.value(index);
            if (SectionOf(action) != section)
            {
                continue;
            }

            if (!wholeSection && !MatchesFilter(action, index, needle))
            {
                continue;
            }

            content.append({RowKind::Action, section, index});
        }

        // A search that finds nothing in a section does not show the section: the list would be all headings.
        if (content.isEmpty() && !needle.isEmpty())
        {
            continue;
        }

        m_rows.append({RowKind::Section, section, -1});
        m_rows.append(content);
    }

    m_hoveredSectionRow = -1;

    QTableWidget *table = ui->shortcutsTable;
    table->clearContents();
    table->setRowCount(m_rows.size());

    for (int row = 0; row < m_rows.size(); row++)
    {
        const Row &r = m_rows.at(row);

        if (r.kind == RowKind::Section)
        {
            auto *item = new QTableWidgetItem(SectionName(r.section));
            // A heading is not a line of the list: nothing to edit, nothing to select. A click still reaches it,
            // and the click is what folds the section.
            item->setFlags(Qt::ItemIsEnabled);
            item->setData(kKindRole, static_cast<quint32>(RowKind::Section));

            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);

            item->setToolTip(tr("Section: %1").arg(SectionName(r.section)));
            table->setItem(row, 0, item);
        }
        else
        {
            auto *nameItem = new QTableWidgetItem(VAbstractShortcutManager::ReadableName(m_actions.value(r.shortcutIndex)));
            nameItem->setData(kShortcutIndexRole, r.shortcutIndex);
            nameItem->setData(kKindRole, static_cast<quint32>(RowKind::Action));
            table->setItem(row, 0, nameItem);

            auto *shortcutsItem = new QTableWidgetItem(
                VAbstractShortcutManager::StringListToReadableString(m_transientShortcuts.value(r.shortcutIndex)));
            table->setItem(row, 1, shortcutsItem);
        }

        // The hidden state belongs to the row index and outlives clearContents(): a heading left hidden would
        // stay invisible for good.
        table->setRowHidden(row, false);
    }

    ApplyFold();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::ApplyFold()
{
    // A search unfolds everything: a match hidden inside a folded section would be a search that answers nothing.
    const bool searching = !ui->lineEditSearch->text().trimmed().isEmpty();

    Section current = Section::File;

    for (int row = 0; row < m_rows.size(); row++)
    {
        const Row &r = m_rows.at(row);

        if (r.kind == RowKind::Section)
        {
            current = r.section;

            if (QTableWidgetItem *item = ui->shortcutsTable->item(row, 0))
            {
                item->setIcon(SectionCaret(m_collapsed.contains(current) && !searching, row == m_hoveredSectionRow));
            }

            // A heading is always visible, whatever was hidden before.
            ui->shortcutsTable->setRowHidden(row, false);
            continue;
        }

        ui->shortcutsTable->setRowHidden(row, !searching && m_collapsed.contains(current));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::HoverSectionRow(int row)
{
    if (row == m_hoveredSectionRow)
    {
        return;
    }

    const int previous = m_hoveredSectionRow;
    m_hoveredSectionRow = row;

    // Only a heading wears a caret, so only a heading has anything to repaint.
    if (previous >= 0 && previous < m_rows.size() && m_rows.at(previous).kind == RowKind::Section)
    {
        RepaintSectionCaret(previous);
    }

    if (row >= 0 && row < m_rows.size() && m_rows.at(row).kind == RowKind::Section)
    {
        RepaintSectionCaret(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::RepaintSectionCaret(int row)
{
    const bool searching = !ui->lineEditSearch->text().trimmed().isEmpty();
    const bool folded = m_collapsed.contains(m_rows.at(row).section) && !searching;

    if (QTableWidgetItem *item = ui->shortcutsTable->item(row, 0))
    {
        item->setIcon(SectionCaret(folded, row == m_hoveredSectionRow));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::ToggleSection(int row)
{
    const Section section = m_rows.at(row).section;

    if (m_collapsed.contains(section))
    {
        m_collapsed.remove(section);
    }
    else
    {
        m_collapsed.insert(section);
    }

    SaveFoldedSections();
    ApplyFold();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::LoadFoldedSections()
{
    m_collapsed.clear();

    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    if (settings == nullptr)
    {
        return;
    }

    const QStringList keys = settings->value(QStringLiteral("shortcuts/foldedSections")).toStringList();

    for (const QString &key : keys)
    {
        for (int s = 0; s < static_cast<int>(Section::LAST_ONE_DO_NOT_USE); s++)
        {
            const auto section = static_cast<Section>(s);
            if (SectionKey(section) == key)
            {
                m_collapsed.insert(section);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::SaveFoldedSections() const
{
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    if (settings == nullptr)
    {
        return;
    }

    QStringList keys;
    for (int s = 0; s < static_cast<int>(Section::LAST_ONE_DO_NOT_USE); s++)
    {
        const auto section = static_cast<Section>(s);
        if (m_collapsed.contains(section))
        {
            keys.append(SectionKey(section));
        }
    }

    settings->setValue(QStringLiteral("shortcuts/foldedSections"), keys);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::UpdateShortcutsTable()
{
    for (int row = 0; row < m_rows.size(); row++)
    {
        const Row &r = m_rows.at(row);
        if (r.kind != RowKind::Action)
        {
            continue;
        }

        if (QTableWidgetItem *item = ui->shortcutsTable->item(row, 1))
        {
            item->setText(
                VAbstractShortcutManager::StringListToReadableString(m_transientShortcuts.value(r.shortcutIndex)));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::MatchesFilter(VShortcutAction action, int shortcutIndex,
                                             const QString &needle) const -> bool
{
    if (needle.isEmpty())
    {
        return true;
    }

    if (VAbstractShortcutManager::ReadableName(action).contains(needle, Qt::CaseInsensitive))
    {
        return true;
    }

    // The technical key too, so that an English word finds a French row: "spread" answers « Couper et pivoter ».
    if (VAbstractShortcutManager::ShortcutActionToString(action).contains(needle, Qt::CaseInsensitive))
    {
        return true;
    }

    // And the shortcut itself: typing "V" finds the tools asked with a V.
    const QString shortcuts =
        VAbstractShortcutManager::StringListToReadableString(m_transientShortcuts.value(shortcutIndex));
    return shortcuts.contains(needle, Qt::CaseInsensitive);
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::SectionOf(VShortcutAction action) -> Section
{
    switch (action)
    {
        // File
        case VShortcutAction::New:
        case VShortcutAction::Open:
        case VShortcutAction::Save:
        case VShortcutAction::SaveAs:
        case VShortcutAction::Quit:
            return Section::File;
        // Edit
        case VShortcutAction::Undo:
        case VShortcutAction::Redo:
            return Section::Edit;
        // View
        case VShortcutAction::ZoomIn:
        case VShortcutAction::ZoomOut:
        case VShortcutAction::ZoomOriginal:
        case VShortcutAction::ZoomFitBest:
        case VShortcutAction::ZoomFitBestCurrent:
        case VShortcutAction::IncreaseLabelFont:
        case VShortcutAction::DecreaseLabelFont:
        case VShortcutAction::OriginalLabelFont:
        case VShortcutAction::ShowAllLabels:
        case VShortcutAction::TableOfVariables:
            return Section::View;
        // Pieces
        case VShortcutAction::NewPatternPiece:
        case VShortcutAction::NextPatternPiece:
        case VShortcutAction::PreviusPatternPiece:
        case VShortcutAction::SavePiece:
        case VShortcutAction::FinalMeasurements:
        case VShortcutAction::CurveDetails:
            return Section::Pieces;
        // Modes
        case VShortcutAction::DrawMode:
        case VShortcutAction::DetailsMode:
        case VShortcutAction::GradedMode:
        case VShortcutAction::ExportMode:
            return Section::Modes;
        // Activating the last tool again is not a mode: it is one of the tools' own gestures.
        case VShortcutAction::LastTool:
            return Section::Tools;
        // Export
        case VShortcutAction::ExportTemplates:
        case VShortcutAction::ExportImage:
        case VShortcutAction::ExportDrawAs:
        case VShortcutAction::ExportDetailsAs:
            return Section::Export;
        case VShortcutAction::LAST_ONE_DO_NOT_USE:
            break;
        default:
            // Every tool is here. One section for all of them, on purpose: the groups of the toolbar belong to
            // the reader, and a second grouping would drift from hers.
            return Section::Tools;
    }

    return Section::Tools;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::SectionName(Section section) -> QString
{
    switch (section)
    {
        case Section::File:
            return tr("File");
        case Section::Edit:
            return tr("Edit");
        case Section::View:
            return tr("View");
        case Section::Pieces:
            return tr("Pieces");
        case Section::Modes:
            return tr("Modes");
        case Section::Tools:
            return tr("Tools");
        case Section::Export:
            return tr("Export");
        case Section::LAST_ONE_DO_NOT_USE:
            break;
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::SectionKey(Section section) -> QString
{
    switch (section)
    {
        case Section::File:
            return QStringLiteral("file");
        case Section::Edit:
            return QStringLiteral("edit");
        case Section::View:
            return QStringLiteral("view");
        case Section::Pieces:
            return QStringLiteral("pieces");
        case Section::Modes:
            return QStringLiteral("modes");
        case Section::Tools:
            return QStringLiteral("tools");
        case Section::Export:
            return QStringLiteral("export");
        case Section::LAST_ONE_DO_NOT_USE:
            break;
    }

    return {};
}
