/************************************************************************
 **
 **  @file   otemplatelanguagerow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The display languages of a template, and the line that edits them.
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

#ifndef OTEMPLATELANGUAGEROW_H
#define OTEMPLATELANGUAGEROW_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;

/**
 * @brief One combo per display language, with the line that adds and removes them.
 *
 * **Both families edit this the same way**, and neither has anything to add to it: a cover page
 * written in two languages and a cartouche written in two languages are the same two languages,
 * chosen the same way. So the row lives in the shared socle and each editor holds one.
 *
 * **It holds what it is given and reports what it changes.** The template's languages are the
 * template's; this widget keeps its own list, and every gesture leaves as one signal carrying the
 * whole list. The editor writes it where it belongs and decides whether that is a modification.
 *
 * **Three rules live here, because they are about the row and not about a family:**
 * - a template always keeps at least one language — there would be no way back from the last
 *   removal, and a page written in none is not a template;
 * - the same language is never offered twice, so a language already chosen is greyed out in the
 *   other combos rather than refused after the fact;
 * - adding takes the first language nobody has taken, in the application's own order, rather than
 *   the newest at the end of a list nobody reads in order.
 *
 * The label and the hint above the combos differ from one family to the next — « Block display
 * language », « Cover page language » — so they are set by the family, which is where the words
 * and their translations live.
 */
class OTemplateLanguageRow : public QWidget
{
    Q_OBJECT

public:
    explicit OTemplateLanguageRow(QWidget *parent = nullptr);

    /// Shows these languages, in this order. An empty list falls back to the application's own.
    void SetLanguages(const QStringList &codes);
    auto Languages() const -> QStringList;

    /// Words the row for a family: its label, and the hint beside it.
    void SetLabels(const QString &label, const QString &hint);

    /// Re-resolves the row's own strings — the label and the hint are the family's to re-word.
    void RetranslateUI();

signals:
    /// The user changed the languages. The list is what the row holds now, never empty.
    void LanguagesChanged(const QStringList &codes);

private:
    /// Rebuilds every combo and the buttons from the languages the row holds.
    void Rebuild();

    void OnLanguageChanged();
    void OnAddLanguage();
    void OnRemoveLanguage(const QString &code);

    QLabel *m_label{nullptr};
    QLabel *m_hint{nullptr};
    QHBoxLayout *m_combosLayout{nullptr};
    QList<QComboBox *> m_combos;
    QPushButton *m_addBtn{nullptr};

    QStringList m_languages;

    /// Silences the combos while the row fills them in: that is not the user choosing.
    bool m_updating{false};
};

#endif // OTEMPLATELANGUAGEROW_H
