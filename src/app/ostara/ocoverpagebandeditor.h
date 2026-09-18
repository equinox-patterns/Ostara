/************************************************************************
 **
 **  @file   ocoverpagebandeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One band of a cover page, as the editor shows it and acts on it.
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

#ifndef OCOVERPAGEBANDEDITOR_H
#define OCOVERPAGEBANDEDITOR_H

#include "core/ocoverpagetemplate.h"

#include <QStringList>
#include <QWidget>

class QGroupBox;
class QLabel;
class QListWidget;
class QMenu;
class QPushButton;

/**
 * @brief One band of the page: its name, the caret that folds it away, the line that acts on it,
 *        and the list it holds.
 *
 * **It holds no informations.** The template does, and this widget never writes into it: every
 * gesture leaves as a signal — add, up, down, remove, select — and the editor decides what it
 * means. That is what a band can be one widget among four while the order, the single selection
 * and the modified state stay with the editor, where they are about the template and not about a
 * list on screen.
 *
 * **It knows the catalogue, and only to name things.** What an information is called comes from
 * there, as it does everywhere else; the band never decides what exists. Naming is also why the
 * caret and the name are here rather than in the editor: a band says where it sits on the page,
 * and that is a fact about the band.
 *
 * **Folded to begin with.** A band is opened when there is something to put in it and closed again
 * when the template is read as a whole — four names. Folding is the editor's own affair: what a
 * band holds is unchanged, and the page is drawn the same either way.
 */
class OCoverPageBandEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OCoverPageBandEditor(OCoverPageTemplate::Placement placement, QWidget *parent = nullptr);

    /// Re-resolves this widget's strings, the band's name, and the entries of the add menu.
    void RetranslateUI();

    /// Shows the informations the band holds, in the order it holds them, by their type id.
    void SetInformations(const QStringList &typeIds);

    /**
     * @brief Selects a row, or none with -1, without saying so.
     *
     * The caller is asking for a row it has already decided on — an information just added, or the
     * one that took the place of a removed one. Reporting it back would be the widget answering a
     * question the caller asked itself; the caller says what the row means by calling what it
     * means.
     */
    void SetCurrentRow(int row);

    /// The row the user has selected, or -1. What the arrows and the bin act on.
    auto CurrentRow() const -> int;

    /// How many informations the band shows — the editor reads it to know where to land.
    auto Count() const -> int;

    /// The bin removes what is selected, so it has nothing to do while nothing is.
    void SetRemoveEnabled(bool enabled);

signals:
    /// The user chose an information to add, by its catalogue id.
    void AddRequested(const QString &typeId);
    void MoveUpRequested();
    void MoveDownRequested();
    void RemoveRequested();

    /// The user chose a row — or let the selection go, which is a row of -1.
    void RowSelected(int row);

private:
    /// Where this band sits on the page, in the editor's own words.
    auto Name() const -> QString;

    /// Fills the add menu from the catalogue: adding an information is adding an entry to it.
    void RebuildAddMenu();

    OCoverPageTemplate::Placement m_placement;

    QGroupBox *m_box{nullptr};
    QPushButton *m_toggleBtn{nullptr};
    QLabel *m_nameLabel{nullptr};
    QPushButton *m_addBtn{nullptr};
    QMenu *m_addMenu{nullptr};
    QPushButton *m_upBtn{nullptr};
    QPushButton *m_downBtn{nullptr};
    QPushButton *m_removeBtn{nullptr};
    QListWidget *m_list{nullptr};

    /// Silences the selection while the widget moves it itself.
    bool m_updatingSelection{false};
};

#endif // OCOVERPAGEBANDEDITOR_H
