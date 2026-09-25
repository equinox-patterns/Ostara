/************************************************************************
 **
 **  @file   opatternpropertieswidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Pattern work fields (pattern name, description, notes) shown in
 **          the Draw property block when no tool is selected.
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

#ifndef OPATTERNPROPERTIESWIDGET_H
#define OPATTERNPROPERTIESWIDGET_H

#include <QPointer>
#include <QWidget>

class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTimer;
class VAbstractPattern;

/**
 * @brief Pattern-level work fields for the Draw property block.
 *
 * Owns the editing surface of three document fields: pattern name,
 * description and notes. One field, one source (VAbstractPattern);
 * the widget reads/writes it and refreshes on document changes.
 */
class OPatternPropertiesWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OPatternPropertiesWidget(QWidget *parent = nullptr);

    /** Reload the fields from the current pattern document. */
    void Refresh();

    /** Re-apply the four labels from the translator installed right now. */
    void RetranslateUI();

protected:
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void CommitPatternName();
    void ScheduleCommit();
    void CommitDelayed();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPatternPropertiesWidget) // NOLINT

    auto CurrentDocument() const -> VAbstractPattern *;
    void Populate();

    QPointer<VAbstractPattern> m_doc{};
    // ⚠️ **Ces quatre-là sont des membres, et c'est tout l'objet du correctif.** Ils étaient
    // construits anonymes dans le constructeur : rien ne gardait de référence, donc rien ne
    // pouvait réécrire leurs textes, et ils restaient dans la langue du premier affichage pour
    // toute la vie de la fenêtre. Cette page est celle que le dock Options montre par défaut en
    // mode Draw, aucun outil sélectionné — donc celle qu'on voit le plus.
    QGroupBox *m_groupBox{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLabel *m_descriptionLabel{nullptr};
    QLabel *m_notesLabel{nullptr};
    QLineEdit *m_patternNameEdit{nullptr};
    QPlainTextEdit *m_descriptionEdit{nullptr};
    QPlainTextEdit *m_notesEdit{nullptr};
    QTimer *m_commitTimer{nullptr};
    bool m_loading{false};
};

#endif // OPATTERNPROPERTIESWIDGET_H
