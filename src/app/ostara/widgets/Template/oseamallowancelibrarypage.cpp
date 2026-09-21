/************************************************************************
 **
 **  @file   oseamallowancelibrarypage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Where the names of the margins are written.
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

#include "oseamallowancelibrarypage.h"

#include "oseamallowancepreview.h"
#include "otemplatelanguagerow.h"

#include "core/oexportunitformat.h"

#include "vmisc/FactoryWidgets.h"
#include "vmisc/def.h"
#include "vmisc/vabstractvalapplication.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
constexpr int kNameColumn = 0;
constexpr int kRuleColumn = 1;

/// The two answers a rule can give, keyed by the token the file keeps — never by the sentence the
/// user reads, which changes with the language while the file must not.
const QString kEqualKey = QStringLiteral("equal");
const QString kAtLeastKey = QStringLiteral("atLeast");
} // namespace

OSeamAllowanceLibraryPage::OSeamAllowanceLibraryPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
    RefreshLanguages();
    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::SetupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // The languages belong to the library and not to a margin, so they stand above everything: they
    // are chosen once and every margin is named in them.
    m_languageRow = new OTemplateLanguageRow();
    connect(m_languageRow, &OTemplateLanguageRow::LanguagesChanged, this,
            &OSeamAllowanceLibraryPage::OnLanguagesChanged);
    layout->addWidget(m_languageRow);

    auto *splitter = new QSplitter(Qt::Horizontal);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addBtn = new QPushButton();
    connect(m_addBtn, &QPushButton::clicked, this, &OSeamAllowanceLibraryPage::OnAdd);
    m_removeBtn = new QPushButton();
    connect(m_removeBtn, &QPushButton::clicked, this, &OSeamAllowanceLibraryPage::OnRemove);

    // **The arrows carry no word** — up and down are read from the icon in every language — so a
    // tooltip is where their names live, as on the symbols page, whose look they share.
    m_upBtn = new QPushButton();
    m_upBtn->setObjectName(QStringLiteral("seamUpBtn"));
    connect(m_upBtn, &QPushButton::clicked, this, &OSeamAllowanceLibraryPage::OnMoveUp);

    m_downBtn = new QPushButton();
    m_downBtn->setObjectName(QStringLiteral("seamDownBtn"));
    connect(m_downBtn, &QPushButton::clicked, this, &OSeamAllowanceLibraryPage::OnMoveDown);

    listBar->addWidget(m_addBtn);
    listBar->addWidget(m_removeBtn);
    listBar->addWidget(m_upBtn);
    listBar->addWidget(m_downBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    m_tree = new QTreeWidget();
    m_tree->setColumnCount(2);
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);

    // **Half each, and it stays half**: a margin is read as its name and its rule together, and left
    // to itself the header would give the names everything and cut the rules short — see resizeEvent.
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(kNameColumn, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(kRuleColumn, QHeaderView::Fixed);

    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this]() { OnSelectionChanged(); });
    leftLayout->addWidget(m_tree, 1);

    m_editGroup = new QGroupBox();
    auto *form = new QFormLayout(m_editGroup);

    m_nameLabel = new QLabel();
    m_nameTabs = new QTabWidget();
    form->addRow(m_nameLabel, m_nameTabs);

    // **The default margin is a case of its own.** What it is worth is the application's preference
    // and not a number written here, so the rule is not asked of it — and the user is told what
    // ticking the box commits them to rather than left to guess.
    m_defaultBox = new QCheckBox();
    connect(m_defaultBox, &QCheckBox::toggled, this, [this]() { OnDefaultToggled(); });
    form->addRow(m_defaultBox);

    m_defaultHint = new QLabel();
    m_defaultHint->setObjectName(QStringLiteral("seamDefaultHint"));
    m_defaultHint->setWordWrap(true);
    form->addRow(m_defaultHint);

    m_ruleLabel = new QLabel();
    auto *rule = new QWidget();
    auto *ruleLayout = new QHBoxLayout(rule);
    ruleLayout->setContentsMargins(0, 0, 0, 0);
    ruleLayout->setSpacing(6);

    m_operatorCombo = MakeComboBox(rule);
    m_operatorCombo->addItem(QString(), kEqualKey);
    m_operatorCombo->addItem(QString(), kAtLeastKey);
    connect(m_operatorCombo, &QComboBox::currentIndexChanged, this, [this]() {
        if (!m_updating)
        {
            WriteCurrentEntry();
        }
    });
    ruleLayout->addWidget(m_operatorCombo);

    m_valueEdit = new QLineEdit(rule);
    connect(m_valueEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_updating)
        {
            WriteCurrentEntry();
        }
    });
    ruleLayout->addWidget(m_valueEdit, 1);

    m_valueUnit = new QLabel(rule);
    ruleLayout->addWidget(m_valueUnit);

    form->addRow(m_ruleLabel, rule);

    leftLayout->addWidget(m_editGroup);
    splitter->addWidget(left);

    m_preview = new OSeamAllowancePreview();
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter, 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::Show()
{
    m_library = OSeamAllowanceLibrary::Load();
    RefreshLanguages();
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::RetranslateUI()
{
    m_addBtn->setText(tr("Add margin"));
    m_removeBtn->setText(tr("Remove margin"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));
    m_tree->setHeaderLabels({tr("Margin"), tr("Recognised by")});
    m_editGroup->setTitle(tr("Selected margin"));
    m_nameLabel->setText(tr("Name"));
    m_ruleLabel->setText(tr("Width"));

    // The two answers, written as a reader would say them — but the values they carry are the
    // tokens the file keeps.
    const int equalIndex = m_operatorCombo->findData(kEqualKey);
    const int atLeastIndex = m_operatorCombo->findData(kAtLeastKey);
    if (equalIndex >= 0)
    {
        m_operatorCombo->setItemText(equalIndex, tr("is exactly"));
    }
    if (atLeastIndex >= 0)
    {
        m_operatorCombo->setItemText(atLeastIndex, tr("is at least"));
    }

    // "Default" says nothing on its own, and what it means is the whole point: the hint is written
    // out rather than folded into a tooltip nobody opens.
    m_defaultBox->setText(tr("This is the default margin"));
    m_defaultBox->setToolTip(tr("The margin a pattern keeps when nothing has been changed on it."));
    m_defaultHint->setText(tr("The default margin is the width set in the preferences (Pattern page). "
                              "Its line appears only on patterns whose pieces really are cut with "
                              "that width."));

    m_valueUnit->setText(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));

    m_languageRow->SetLabels(tr("Margin languages"), tr("Every margin is named in all of them"));
    m_languageRow->RetranslateUI();

    RefreshLanguages();
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::RefreshLanguages()
{
    m_updating = true;

    // The row refuses to let the list be emptied: a library written in no language is a library with
    // nowhere to write, so what comes back is what will be written in.
    m_languageRow->SetLanguages(m_library.languages);
    m_library.languages = m_languageRow->Languages();

    m_nameTabs->clear();
    m_nameEdits.clear();

    for (const QString &code : m_library.languages)
    {
        auto *edit = new QLineEdit();
        connect(edit, &QLineEdit::textEdited, this, [this]() {
            if (!m_updating)
            {
                WriteCurrentEntry();
            }
        });
        m_nameEdits.append(edit);
        // The tab carries the language's own name, not the code the file keeps: the user chose a
        // language, not an identifier, and the library's stored code is nobody's business.
        m_nameTabs->addTab(edit, QLocale(code).nativeLanguageName());
    }

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::RefreshList()
{
    const int wanted = CurrentRow();

    m_updating = true;
    m_tree->clear();
    for (const OSeamAllowanceLibrary::Entry &entry : m_library.entries)
    {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(kNameColumn, entry.texts.value(m_library.languages.value(0)));
        item->setText(kRuleColumn,
                      entry.byDefault ? tr("default")
                                      : (entry.condition.comparison == OSeamAllowanceLibrary::Comparison::AtLeast
                                             ? tr("from %1 on")
                                             : tr("exactly %1"))
                                            .arg(OExportUnitFormat::Format(
                                                     entry.condition.mm, VAbstractValApplication::VApp()->patternUnits())
                                                 + QLatin1Char(' ')
                                                 + UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
    }
    m_updating = false;

    Select(wanted >= 0 && wanted < m_tree->topLevelItemCount() ? wanted : -1);
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::RefreshEditor()
{
    const int row = CurrentRow();
    const bool selected = row >= 0 && row < m_library.entries.size();

    m_editGroup->setEnabled(selected);
    if (!selected)
    {
        m_preview->Clear();
        return;
    }

    const OSeamAllowanceLibrary::Entry &entry = m_library.entries.at(row);

    m_updating = true;
    for (int i = 0; i < m_nameEdits.size(); ++i)
    {
        const QString code = m_library.languages.value(i);
        m_nameEdits.at(i)->setText(entry.texts.value(code));
    }

    m_defaultBox->setChecked(entry.byDefault);
    m_operatorCombo->setCurrentIndex(m_operatorCombo->findData(
        entry.condition.comparison == OSeamAllowanceLibrary::Comparison::AtLeast ? kAtLeastKey
                                                                                : kEqualKey));
    m_updating = false;

    const Unit unit = VAbstractValApplication::VApp()->patternUnits();
    m_valueEdit->setText(OExportUnitFormat::Format(entry.condition.mm, unit));

    RefreshConditionEnabled();

    m_preview->SetEntry(m_nameEdits.value(0) != nullptr ? m_nameEdits.at(0)->text() : QString(),
                        entry.byDefault, entry.condition.comparison, entry.condition.mm, unit);
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::RefreshConditionEnabled()
{
    // **A default margin has no rule to write.** What it is worth is a preference, and a field the
    // user could fill in would be a field that quietly did nothing.
    const bool byDefault = m_defaultBox->isChecked();
    m_operatorCombo->setEnabled(!byDefault);
    m_valueEdit->setEnabled(!byDefault);
}

//---------------------------------------------------------------------------------------------------------------------

auto OSeamAllowanceLibraryPage::CurrentRow() const -> int
{
    const QTreeWidgetItem *item = m_tree->currentItem();
    return item == nullptr ? -1 : m_tree->indexOfTopLevelItem(const_cast<QTreeWidgetItem *>(item));
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::Select(int row)
{
    m_updating = true;
    m_tree->setCurrentItem(row >= 0 ? m_tree->topLevelItem(row) : nullptr);
    m_updating = false;
    RefreshEditor();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnSelectionChanged()
{
    if (!m_updating)
    {
        RefreshEditor();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnLanguagesChanged(const QStringList &codes)
{
    if (m_updating)
    {
        return;
    }

    m_library.languages = codes;
    RefreshLanguages();
    RefreshEditor();
    Save();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnDefaultToggled()
{
    if (m_updating)
    {
        return;
    }

    // **At most one default**, which is what "the default" means: ticking the box on one margin
    // takes it off every other, and the list is redrawn to show it.
    if (m_defaultBox->isChecked())
    {
        for (int i = 0; i < m_library.entries.size(); ++i)
        {
            if (i != CurrentRow())
            {
                m_library.entries[i].byDefault = false;
            }
        }
    }

    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

auto OSeamAllowanceLibraryPage::WidthFromWidgets() const -> qreal
{
    bool ok = false;
    const qreal mm = OExportUnitFormat::Parse(m_valueEdit->text(),
                                              VAbstractValApplication::VApp()->patternUnits(), &ok);
    return ok ? mm : 0.0;
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::WriteCurrentEntry()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size())
    {
        return;
    }

    OSeamAllowanceLibrary::Entry &entry = m_library.entries[row];
    entry.texts.clear();
    for (int i = 0; i < m_nameEdits.size(); ++i)
    {
        const QString code = m_library.languages.value(i);
        const QString text = m_nameEdits.at(i)->text();
        if (!text.isEmpty())
        {
            entry.texts.insert(code, text);
        }
    }

    entry.byDefault = m_defaultBox->isChecked();
    entry.condition.comparison =
        m_operatorCombo->currentData().toString() == kAtLeastKey
            ? OSeamAllowanceLibrary::Comparison::AtLeast
            : OSeamAllowanceLibrary::Comparison::Equal;
    entry.condition.mm = WidthFromWidgets();

    if (Save())
    {
        const int selected = row;
        RefreshList();
        Select(selected);
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OSeamAllowanceLibraryPage::Save() -> bool
{
    QString error;
    if (!OSeamAllowanceLibrary::Save(m_library, &error))
    {
        emit StatusMessage(error);
        return false;
    }

    // Whoever shows these margins is showing what was on disk a moment ago — see the signal.
    emit LibraryChanged();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnAdd()
{
    OSeamAllowanceLibrary::Entry entry;
    // A new margin starts with a name the user can recognise as theirs to change, written in every
    // language of the library rather than in one they may not be reading.
    for (const QString &code : m_library.languages)
    {
        entry.texts.insert(code, tr("New margin"));
    }
    // A rule and not a default: what starts as the default margin is the one the library was seeded
    // with, and two of them would be two answers to the same question.
    entry.byDefault = false;
    entry.condition.comparison = OSeamAllowanceLibrary::Comparison::Equal;
    entry.condition.mm = 0.0;

    m_library.entries.append(entry);
    const int added = static_cast<int>(m_library.entries.size()) - 1;
    if (Save())
    {
        RefreshList();
        Select(added);
        emit StatusMessage(tr("Margin added."));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnRemove()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size())
    {
        return;
    }

    m_library.entries.removeAt(row);
    if (Save())
    {
        RefreshList();
        Select(row < m_library.entries.size() ? row : static_cast<int>(m_library.entries.size()) - 1);
        emit StatusMessage(tr("Margin removed."));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnMoveUp()
{
    const int row = CurrentRow();
    if (row <= 0 || row >= m_library.entries.size())
    {
        return;
    }

    m_library.entries.swapItemsAt(row, row - 1);
    if (Save())
    {
        RefreshList();
        Select(row - 1);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::OnMoveDown()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size() - 1)
    {
        return;
    }

    m_library.entries.swapItemsAt(row, row + 1);
    if (Save())
    {
        RefreshList();
        Select(row + 1);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowanceLibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Half each, as the symbols page holds its two columns: neither the name nor the rule is worth
    // more room than the other.
    const int half = m_tree->viewport()->width() / 2;
    m_tree->setColumnWidth(kNameColumn, half);
    m_tree->setColumnWidth(kRuleColumn, half);
}
