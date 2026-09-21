/************************************************************************
 **
 **  @file   osymbollibrarypage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The page a symbol library is kept from — the marks a legend is
 **          written with, and what each one says.
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

#include "osymbollibrarypage.h"

#include "osymbolpreview.h"
#include "otemplatelanguagerow.h"

#include "core/osymbolshapes.h"

#include "vmisc/FactoryWidgets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
constexpr int kShapeColumn = 0;
constexpr int kTextColumn = 1;

//---------------------------------------------------------------------------------------------------------------------
/// Where a shape sits in the list of all of them — which is also its row in the combo.
auto IndexOfShape(const QVector<OSymbolShapes::Shape> &shapes, const OSymbolShapes::Shape &shape) -> int
{
    for (int i = 0; i < shapes.size(); ++i)
    {
        if (shapes.at(i) == shape)
        {
            return i;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
/// The marks' names, one after the other — what the list says a symbol is made of.
auto MarkNames(const QVector<OSymbolShapes::Shape> &shapes) -> QString
{
    QStringList names;
    names.reserve(shapes.size());
    for (const OSymbolShapes::Shape &shape : shapes)
    {
        names.append(OSymbolShapes::Name(shape));
    }
    return names.join(QStringLiteral(", "));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OSymbolLibraryPage::OSymbolLibraryPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
    RefreshLanguages();
    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::SetupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // The languages belong to the library and not to a symbol, so they stand above everything the
    // page holds rather than inside the editing group: they are chosen once and every symbol is
    // written in them.
    m_languageRow = new OTemplateLanguageRow();
    connect(m_languageRow, &OTemplateLanguageRow::LanguagesChanged, this,
            &OSymbolLibraryPage::OnLanguagesChanged);
    layout->addWidget(m_languageRow);

    auto *splitter = new QSplitter(Qt::Horizontal);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addBtn = new QPushButton();
    connect(m_addBtn, &QPushButton::clicked, this, &OSymbolLibraryPage::OnAdd);
    m_removeBtn = new QPushButton();
    connect(m_removeBtn, &QPushButton::clicked, this, &OSymbolLibraryPage::OnRemove);

    // **The arrows carry no word.** Up and down are read from the icon in every language, and the two
    // together are read as a pair; a name is still worth having, and a tooltip is where it goes.
    m_upBtn = new QPushButton();
    m_upBtn->setObjectName(QStringLiteral("symbolUpBtn"));
    connect(m_upBtn, &QPushButton::clicked, this, &OSymbolLibraryPage::OnMoveUp);

    m_downBtn = new QPushButton();
    m_downBtn->setObjectName(QStringLiteral("symbolDownBtn"));
    connect(m_downBtn, &QPushButton::clicked, this, &OSymbolLibraryPage::OnMoveDown);
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

    // **Half each, and it stays half.** A symbol is read as its marks and its word together, and
    // neither half is worth more room than the other; left to itself the header would give the marks'
    // names everything and cut the words short. The halves are set on every resize — see resizeEvent.
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(kShapeColumn, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(kTextColumn, QHeaderView::Fixed);

    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this]() { OnSelectionChanged(); });
    leftLayout->addWidget(m_tree, 1);

    m_editGroup = new QGroupBox();
    auto *form = new QFormLayout(m_editGroup);

    m_marksLabel = new QLabel();

    // **One row per mark, and while a symbol is selected the rows are the truth**: adding and
    // removing a mark is a gesture on these widgets, and the entry is written from them. What they
    // are kept in is their order, because that is the order the legend will be read in.
    auto *marks = new QWidget();
    m_marksLayout = new QVBoxLayout(marks);
    m_marksLayout->setContentsMargins(0, 0, 0, 0);
    m_marksLayout->setSpacing(2);

    m_addMarkBtn = new QPushButton();
    connect(m_addMarkBtn, &QPushButton::clicked, this, &OSymbolLibraryPage::OnAddMark);
    m_marksLayout->addWidget(m_addMarkBtn);

    form->addRow(m_marksLabel, marks);

    m_textLabel = new QLabel();
    m_textTabs = new QTabWidget();
    form->addRow(m_textLabel, m_textTabs);

    // The two rows that say whether the symbol belongs to every page or waits to be asked. The hint
    // is not decoration: "permanent" means nothing on its own, and the user is the one who has to
    // know what it commits them to.
    m_permanentBox = new QCheckBox();
    connect(m_permanentBox, &QCheckBox::toggled, this, [this]() { OnPermanentChanged(); });
    form->addRow(m_permanentBox);

    m_permanentHint = new QLabel();
    m_permanentHint->setObjectName(QStringLiteral("symbolPermanentHint"));
    m_permanentHint->setWordWrap(true);
    form->addRow(m_permanentHint);

    leftLayout->addWidget(m_editGroup);

    splitter->addWidget(left);

    m_preview = new OSymbolPreview();
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter, 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::Show()
{
    m_library = OSymbolLibrary::Load();
    RefreshLanguages();
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RetranslateUI()
{
    m_addBtn->setText(tr("Add symbol"));
    m_removeBtn->setText(tr("Remove symbol"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));
    m_tree->setHeaderLabels({tr("Symbol"), tr("Text")});
    m_editGroup->setTitle(tr("Selected symbol"));
    m_marksLabel->setText(tr("Marks"));
    m_addMarkBtn->setText(tr("Add another mark"));
    m_textLabel->setText(tr("Text"));

    // "Permanent" says nothing by itself, and what it commits the user to is the whole point: the
    // hint is therefore written out rather than folded into a tooltip nobody opens.
    m_permanentBox->setText(tr("Permanent"));
    m_permanentBox->setToolTip(tr("Shown on every cover page that uses this template."));
    m_permanentHint->setText(tr("A permanent symbol appears on every cover page that uses this "
                                "template, without being asked for. The others are added page by page, "
                                "where they are wanted."));

    m_languageRow->SetLabels(tr("Symbol languages"), tr("Every symbol is written in all of them"));
    m_languageRow->RetranslateUI();

    // The marks are named by the application and those names translate, so the rows are filled again
    // rather than left in whatever language the combos were first built in — which the selection
    // does on its own, see RefreshEditor.
    //
    // The empty list is a sentence, and the tab names are languages' own.
    RefreshLanguages();
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RefreshLanguages()
{
    m_updating = true;

    // The row refuses to let the list be emptied: a library written in no language is a library with
    // nowhere to write, so what comes back is what will be written in.
    m_languageRow->SetLanguages(m_library.languages);
    m_library.languages = m_languageRow->Languages();

    m_textTabs->clear();
    m_textEdits.clear();

    for (const QString &code : m_library.languages)
    {
        auto *edit = new QLineEdit();
        connect(edit, &QLineEdit::textEdited, this, [this]() { OnTextEdited(); });

        // One tab per language, each named in that language — a language is not translated into
        // another one, and the person who writes in it should not have to find it under a name they
        // do not read.
        m_textTabs->addTab(edit, QLocale(code).nativeLanguageName());
        m_textEdits.append(edit);
    }

    m_updating = false;

    RefreshEditor();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RefreshList()
{
    m_updating = true;

    m_tree->clear();

    if (m_library.entries.isEmpty())
    {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(kShapeColumn, tr("No symbols yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
    }
    else
    {
        // A row is an entry: the list is rebuilt from the library in the library's own order, which
        // is the order the legend will be read in.
        for (const OSymbolLibrary::Entry &entry : m_library.entries)
        {
            auto *item = new QTreeWidgetItem(m_tree);
            item->setText(kShapeColumn, MarkNames(entry.shapes));
            item->setText(kTextColumn, entry.texts.value(m_library.languages.value(0)));
        }
    }

    m_tree->setCurrentItem(nullptr);
    m_updating = false;

    OnSelectionChanged();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RefreshEditor()
{
    m_updating = true;

    const int row = CurrentRow();

    if (row < 0 || row >= m_library.entries.size())
    {
        // Nothing to edit. The fields are emptied rather than left showing another symbol's words,
        // which would be the page offering to write them somewhere else.
        ClearMarkRows();
        for (QLineEdit *edit : m_textEdits)
        {
            edit->clear();
        }
        m_permanentBox->setChecked(false);
        m_preview->Clear();
        m_updating = false;
        return;
    }

    const OSymbolLibrary::Entry &entry = m_library.entries.at(row);

    RebuildMarkRows(entry.shapes);
    for (int i = 0; i < m_textEdits.size() && i < m_library.languages.size(); ++i)
    {
        m_textEdits.at(i)->setText(entry.texts.value(m_library.languages.at(i)));
    }
    m_permanentBox->setChecked(entry.permanent);
    m_preview->SetSymbol(entry.shapes, entry.texts.value(m_library.languages.value(0)));

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------

auto OSymbolLibraryPage::CurrentRow() const -> int
{
    return m_tree->currentIndex().row();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::Select(int row)
{
    if (row >= 0 && row < m_library.entries.size())
    {
        m_tree->setCurrentItem(m_tree->topLevelItem(row));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnSelectionChanged()
{
    if (m_updating)
    {
        return;
    }

    const int row = CurrentRow();
    const bool hasEntry = row >= 0 && row < m_library.entries.size();

    // A symbol is edited, so there is nothing to edit until one is picked — and the marks that move
    // it are only offered where they would do something.
    m_editGroup->setEnabled(hasEntry);
    m_removeBtn->setEnabled(hasEntry);
    m_upBtn->setEnabled(hasEntry && row > 0);
    m_downBtn->setEnabled(hasEntry && row < m_library.entries.size() - 1);

    RefreshEditor();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnLanguagesChanged(const QStringList &codes)
{
    if (m_updating)
    {
        return;
    }

    // **A language taken out of the list keeps its texts.** They are no longer shown, but they were
    // written, and putting the language back gives them back rather than asking for them a second
    // time. Removing a language is a decision about the page, not an order to forget a translation.
    m_library.languages = codes;
    RefreshLanguages();
    Save();
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RebuildMarkRows(const QVector<OSymbolShapes::Shape> &shapes)
{
    ClearMarkRows();

    const QVector<OSymbolShapes::Shape> all = OSymbolShapes::All();

    for (const OSymbolShapes::Shape &shape : shapes)
    {
        auto *row = new QWidget();
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        // Built by the factory and not by hand: a combo has to follow the stylesheet like every
        // other one in the application, and the native macOS popup ignores it.
        QComboBox *combo = MakeComboBox();
        for (const OSymbolShapes::Shape &candidate : all)
        {
            combo->addItem(OSymbolShapes::Name(candidate));
        }
        combo->setCurrentIndex(IndexOfShape(all, shape));
        connect(combo, &QComboBox::currentIndexChanged, this, [this]() { OnShapeChanged(); });
        rowLayout->addWidget(combo, 1);

        auto *removeBtn = new QPushButton();
        removeBtn->setObjectName(QStringLiteral("symbolMarkTrashBtn"));
        removeBtn->setToolTip(tr("Remove this mark"));
        connect(removeBtn, &QPushButton::clicked, this, [this, row]() { OnRemoveMark(row); });
        rowLayout->addWidget(removeBtn);

        // Before the "add" button, which stays at the foot of the list whatever its length.
        m_marksLayout->insertWidget(m_marksLayout->count() - 1, row);
        m_markRows.append(row);
        m_markCombos.append(combo);
    }

    RefreshMarkButtons();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::ClearMarkRows()
{
    for (QWidget *row : m_markRows)
    {
        m_marksLayout->removeWidget(row);
        delete row; // takes the combo and the bin with it
    }
    m_markRows.clear();
    m_markCombos.clear();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::RefreshMarkButtons()
{
    // **One mark always remains.** A symbol that stands for nothing has nothing to draw, and there
    // would be no way back from removing the last one — the rule the language row keeps for its own
    // list. The bin is therefore shown where it would do something, rather than greyed out on every
    // row and read as broken.
    const bool canRemove = m_markRows.size() > 1;

    for (QWidget *row : m_markRows)
    {
        if (auto *button = qobject_cast<QPushButton *>(row->layout()->itemAt(1)->widget()))
        {
            button->setVisible(canRemove);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OSymbolLibraryPage::ShapesFromRows() const -> QVector<OSymbolShapes::Shape>
{
    QVector<OSymbolShapes::Shape> shapes;
    const QVector<OSymbolShapes::Shape> all = OSymbolShapes::All();

    for (QComboBox *combo : m_markCombos)
    {
        const int index = combo->currentIndex();
        if (index >= 0 && index < all.size())
        {
            shapes.append(all.at(index));
        }
    }
    return shapes;
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnAddMark()
{
    QVector<OSymbolShapes::Shape> shapes = ShapesFromRows();

    // **The new mark starts as the one already chosen**, and the first one for a symbol that has
    // none: a second notch is nearly always another notch, and sending the user back to the top of a
    // list of twenty-two every time would make them find their place again for nothing.
    shapes.append(shapes.isEmpty() ? OSymbolShapes::All().constFirst() : shapes.constLast());

    RebuildMarkRows(shapes);
    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnRemoveMark(QWidget *row)
{
    const qsizetype index = m_markRows.indexOf(row);
    if (index < 0 || m_markRows.size() <= 1)
    {
        return;
    }

    QVector<OSymbolShapes::Shape> shapes = ShapesFromRows();
    shapes.removeAt(index);

    RebuildMarkRows(shapes);
    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnShapeChanged()
{
    if (m_updating)
    {
        return;
    }

    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnTextEdited()
{
    if (m_updating)
    {
        return;
    }

    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnAdd()
{
    OSymbolLibrary::Entry entry;

    // **A symbol starts on a mark, not on nothing.** It is a mark and a word, and half of one cannot
    // be drawn; starting on the first mark makes the page show what it is about to become rather
    // than an empty pane the user has to guess at.
    entry.shapes.append(OSymbolShapes::All().constFirst());

    // And it exists in every language the library keeps from the moment it is added, so an empty
    // field means "not written yet" rather than "not in this language".
    for (const QString &code : m_library.languages)
    {
        entry.texts.insert(code, QString());
    }

    m_library.entries.append(entry);

    // Its id is drawn before anything can be written, so that the one session in which nothing has
    // been saved yet is not a session in which the symbol cannot be chosen.
    OSymbolLibrary::GiveIds(m_library);

    if (!Save())
    {
        m_library.entries.removeLast();
        return;
    }

    RefreshList();
    Select(static_cast<int>(m_library.entries.size()) - 1);
    emit StatusMessage(tr("Symbol added"));
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnRemove()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size())
    {
        return;
    }

    const QString name = MarkNames(m_library.entries.at(row).shapes);
    if (QMessageBox::question(this, tr("Symbols"), tr("Remove \"%1\" from the library?").arg(name))
        != QMessageBox::Yes)
    {
        return;
    }

    const OSymbolLibrary::Entry removed = m_library.entries.takeAt(row);
    if (!Save())
    {
        m_library.entries.insert(row, removed);
        return;
    }

    RefreshList();
    Select(qMin(row, static_cast<int>(m_library.entries.size()) - 1));
    emit StatusMessage(tr("Symbol removed"));
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnMoveUp()
{
    const int row = CurrentRow();
    if (row <= 0 || row >= m_library.entries.size())
    {
        return;
    }

    m_library.entries.swapItemsAt(row, row - 1);
    if (!Save())
    {
        m_library.entries.swapItemsAt(row - 1, row);
        return;
    }

    RefreshList();
    Select(row - 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnMoveDown()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size() - 1)
    {
        return;
    }

    m_library.entries.swapItemsAt(row, row + 1);
    if (!Save())
    {
        m_library.entries.swapItemsAt(row + 1, row);
        return;
    }

    RefreshList();
    Select(row + 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::OnPermanentChanged()
{
    if (m_updating)
    {
        return;
    }

    WriteCurrentEntry();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // The halves are recomputed here rather than left to the header: two sections that both want the
    // room would divide it by a rule of their own, and the rule wanted is the plain one — half each.
    const int width = m_tree->viewport()->width();
    m_tree->header()->resizeSection(kShapeColumn, width / 2);
    m_tree->header()->resizeSection(kTextColumn, width - width / 2);
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolLibraryPage::WriteCurrentEntry()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_library.entries.size())
    {
        return;
    }

    OSymbolLibrary::Entry &entry = m_library.entries[row];

    entry.shapes = ShapesFromRows();
    entry.permanent = m_permanentBox->isChecked();

    for (int i = 0; i < m_textEdits.size() && i < m_library.languages.size(); ++i)
    {
        entry.texts.insert(m_library.languages.at(i), m_textEdits.at(i)->text());
    }

    // The list says what was just typed. Left to the next opening it would look as though the page
    // had not listened, and the user would wonder whether their words were kept.
    if (QTreeWidgetItem *item = m_tree->topLevelItem(row))
    {
        m_updating = true;
        item->setText(kShapeColumn, MarkNames(entry.shapes));
        item->setText(kTextColumn, entry.texts.value(m_library.languages.value(0)));
        m_updating = false;
    }

    m_preview->SetSymbol(entry.shapes, entry.texts.value(m_library.languages.value(0)));
    Save();
}

//---------------------------------------------------------------------------------------------------------------------

auto OSymbolLibraryPage::Save() -> bool
{
    QString error;
    if (!OSymbolLibrary::Save(m_library, &error))
    {
        QMessageBox::critical(this, tr("Symbols"), error);
        return false;
    }

    emit LibraryChanged();
    return true;
}
