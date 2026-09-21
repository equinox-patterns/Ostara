/************************************************************************
 **
 **  @file   oseamallowancelibrarypage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Where the names of the margins are written — the library a
 **          cover page reads when it states what a pattern is cut with.
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

#ifndef OSEAMALLOWANCELIBRARYPAGE_H
#define OSEAMALLOWANCELIBRARYPAGE_H

#include "core/oseamallowancelibrary.h"

#include <QWidget>

class OSeamAllowancePreview;
class OTemplateLanguageRow;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;

/**
 * @brief The margins a cover page may name, written once for every pattern.
 *
 * **The page writes names and rules, and never a width taken from a pattern.** What a margin is
 * called is the user's to say; which widths answer to it is a rule they write once — « exactly one
 * and a half centimetres », « from two centimetres on » — and the patterns are read against it
 * afterwards. A library holding the widths of one pattern would name the next one wrongly.
 *
 * **The widths are kept in millimetres**, as the print tab keeps its lengths, and shown in the unit
 * the application is working in: a library shared by patterns measured in centimetres and in inches
 * must mean one thing, and a condition stored in whatever unit was on screen would mean another.
 *
 * The languages are the library's own and stand above everything, as they do in the symbols page:
 * they are chosen once, and every margin is named in all of them.
 *
 * The library is read every time the page is shown and never cached: it is a file the user may have
 * written by hand, and a page showing what it remembered would be a page lying about it.
 */
class OSeamAllowanceLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit OSeamAllowanceLibraryPage(QWidget *parent = nullptr);

    /// Read the library again and show it, with nothing selected.
    void Show();

    void RetranslateUI();

signals:
    /// What the page has just done, for the window's status bar.
    void StatusMessage(const QString &message);

    /// @brief The library was written — see the symbols page, which says the same thing for the same
    /// reason: whoever shows these margins is showing what was on disk a moment ago.
    void LibraryChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void SetupUI();
    void RefreshLanguages();
    void RefreshList();
    void RefreshEditor();
    void RefreshConditionEnabled();

    auto CurrentRow() const -> int;
    void Select(int row);

    void OnSelectionChanged();
    void OnLanguagesChanged(const QStringList &codes);
    void OnDefaultToggled();
    void OnAdd();
    void OnRemove();
    void OnMoveUp();
    void OnMoveDown();

    void WriteCurrentEntry();
    auto Save() -> bool;

    /// The width the entry's widgets hold, brought back to millimetres.
    auto WidthFromWidgets() const -> qreal;

    OSeamAllowanceLibrary::Library m_library;

    OTemplateLanguageRow *m_languageRow = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QPushButton *m_upBtn = nullptr;
    QPushButton *m_downBtn = nullptr;
    QTreeWidget *m_tree = nullptr;
    QGroupBox *m_editGroup = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLabel *m_ruleLabel = nullptr;
    QTabWidget *m_nameTabs = nullptr;
    QVector<QLineEdit *> m_nameEdits; ///< one per language, in the library's own order
    QCheckBox *m_defaultBox = nullptr;
    QLabel *m_defaultHint = nullptr;
    QComboBox *m_operatorCombo = nullptr;
    QLineEdit *m_valueEdit = nullptr;
    QLabel *m_valueUnit = nullptr;
    OSeamAllowancePreview *m_preview = nullptr;

    /// True while the widgets are being filled, so that filling them is not taken for an edit.
    bool m_updating = false;
};

#endif // OSEAMALLOWANCELIBRARYPAGE_H
