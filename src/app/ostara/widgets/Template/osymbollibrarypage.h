/************************************************************************
 **
 **  @file   osymbollibrarypage.h
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

#ifndef OSYMBOLLIBRARYPAGE_H
#define OSYMBOLLIBRARYPAGE_H

#include <QVector>
#include <QWidget>

#include "core/osymbollibrary.h"

class OSymbolPreview;
class OTemplateLanguageRow;

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QVBoxLayout;

/**
 * @brief The symbol library, edited: add a symbol, say what it means, put it in order.
 *
 * **A page, not a dialogue**, and **the cover page editor's own**: a legend is part of a cover page
 * and of nothing else, so this is the only place the library is kept — where the logo library, which
 * both template families draw from, is opened by either editor.
 *
 * **A symbol is permanent, or it waits to be chosen.** Permanent is the state the library is for: a
 * grain line is on every page, and so is what the page's own marks mean. The box says which the
 * symbol being edited is, and the hint beside it says what that means for the page.
 *
 * **The order is the user's.** A legend is read from top to bottom, and what it says first is an
 * argument the library cannot make: entries are moved by hand and kept in the order they were put
 * in, so what the page shows is what the page will print.
 *
 * **A mark is chosen, never drawn.** What a symbol looks like comes from the application's own
 * drawing (@ref OSymbolRender), so the list offers the marks Ostara makes and nothing else.
 *
 * **A symbol may stand for several marks**, and the page lets it: a pattern notched three different
 * ways calls all three of them "notches", so a mark is added to the one already there and the whole
 * list is read as one line of the legend. One mark always remains — a symbol that stands for nothing
 * cannot be drawn — which is the rule the language row keeps for its own list.
 *
 * The library is read every time the page is shown and never cached: it is a file the user may have
 * written by hand, and a page showing what it remembered would be a page lying about it.
 */
class OSymbolLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit OSymbolLibraryPage(QWidget *parent = nullptr);

    /// Read the library again and show it, with nothing selected.
    void Show();

    void RetranslateUI();

signals:
    /// What the page has just done, for the window's status bar.
    void StatusMessage(const QString &message);

    /**
     * @brief The library was written.
     *
     * **Whoever shows these symbols is showing what was on disk a moment ago** — the legend of a
     * cover page, typically, drawn on another page of the same editor. They are told here rather
     * than left to watch for it: a file is only worth reading again once it has changed.
     */
    void LibraryChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void SetupUI();
    void RefreshLanguages();
    void RefreshList();
    void RefreshEditor();

    void RebuildMarkRows(const QVector<OSymbolShapes::Shape> &shapes);
    void ClearMarkRows();
    void RefreshMarkButtons();
    auto ShapesFromRows() const -> QVector<OSymbolShapes::Shape>;

    auto CurrentRow() const -> int;
    void Select(int row);

    void OnSelectionChanged();
    void OnLanguagesChanged(const QStringList &codes);
    void OnShapeChanged();
    void OnTextEdited();
    void OnPermanentChanged();
    void OnAddMark();
    void OnRemoveMark(QWidget *row);
    void OnAdd();
    void OnRemove();
    void OnMoveUp();
    void OnMoveDown();

    void WriteCurrentEntry();
    auto Save() -> bool;

    OSymbolLibrary::Library m_library;

    OTemplateLanguageRow *m_languageRow = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QPushButton *m_upBtn = nullptr;
    QPushButton *m_downBtn = nullptr;
    QTreeWidget *m_tree = nullptr;
    QGroupBox *m_editGroup = nullptr;
    QLabel *m_marksLabel = nullptr;
    QLabel *m_textLabel = nullptr;
    QVBoxLayout *m_marksLayout = nullptr; ///< holds one row per mark
    QPushButton *m_addMarkBtn = nullptr;
    QVector<QWidget *> m_markRows;     ///< a mark's combo and its own bin, in the order they are read
    QVector<QComboBox *> m_markCombos; ///< the combos of @ref m_markRows, in the same order
    QTabWidget *m_textTabs = nullptr;
    QVector<QLineEdit *> m_textEdits; ///< one per language, in the library's own order
    QCheckBox *m_permanentBox = nullptr;
    QLabel *m_permanentHint = nullptr;
    OSymbolPreview *m_preview = nullptr;

    /// True while the widgets are being filled, so that filling them is not taken for an edit.
    bool m_updating = false;
};

#endif // OSYMBOLLIBRARYPAGE_H
