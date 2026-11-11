/************************************************************************
 **
 **  @file   oexportrollformatwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The Roll shape of the print tab — one continuous sheet, and the
 **          fields that go with it.
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

#ifndef OEXPORTROLLFORMATWIDGET_H
#define OEXPORTROLLFORMATWIDGET_H

#include <QWidget>

#include "../vmisc/def.h"

class OExportRowIconButton;
class QCheckBox;
class QLabel;
class QLineEdit;

/**
 * @brief The fields of a Roll model: its width, its page length, and its margins.
 *
 * A roll is one continuous sheet: its height is whatever the pattern needs — the nesting
 * produces it — and nothing can be set about that. What CAN be set is:
 *
 *   - **sa largeur** — 841 mm par défaut, la largeur d'un A0, mais un imprimeur charge aussi du
 *     36 pouces (914 mm) et d'autres rouleaux. C'est la seule dimension d'un format d'impression
 *     que l'utilisatrice saisit, et elle dépend de la machine, pas d'une norme. L'icône d'info
 *     liste les largeurs les plus courantes, en tooltip comme en dialogue.
 *   - **la longueur maximale d'une page** — les machines à plier n'ont pas toutes la même limite ;
 *     au-delà, le pliage est manuel et se paie. Vide veut dire 500 cm, le plafond du PDF.
 *   - **l'export hors marges droite/gauche** — pour que la surface facturée soit celle du document
 *     utile, les deux bandes de marge latérale étant implicites sur un rouleau plus large.
 *
 * ⚠️ **Aucune de ces valeurs ne vient de la table des modèles** : la table donne le défaut de la
 * largeur, et rien d'autre. Le widget garde ce que l'utilisatrice a saisi et le rend tel quel.
 */
class OExportRollFormatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportRollFormatWidget(QWidget *parent = nullptr);

    /// @brief Adopt the width @p widthMm — le défaut du modèle, ou la valeur déjà saisie.
    void SetWidthMm(qreal widthMm);

    /// @brief La marge du document, en millimètres — elle décide la largeur exportée.
    void SetMarginMm(qreal marginMm);

    /// @brief Show the lengths in @p unit from now on.
    void SetUnit(const Unit &unit);

    auto WidthMm() const -> qreal;

    /// @brief La longueur maximale d'une page, en millimètres — **zéro veut dire 500 cm**.
    auto MaxPageLengthMm() const -> qreal;

    auto TrimSideMargins() const -> bool;

signals:
    /// Une des trois valeurs a changé — le dialogue en dépend (largeur, validité, nom).
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    void CommitWidth();
    void CommitMaxLength();
    void Refresh();
    void Retranslate();
    void ShowRollWidths();

    /// @brief Le texte du tooltip ET de la dialogue : une seule liste, deux lectures.
    auto RollWidthsBody() const -> QString;

    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    OExportRowIconButton *m_infoButton{nullptr};

    QLabel *m_maxLengthLabel{nullptr};
    QLineEdit *m_maxLengthEdit{nullptr};

    QCheckBox *m_trimCheck{nullptr};
    QLabel *m_trimHint{nullptr};

    qreal m_widthMm{841.0};
    qreal m_maxPageLengthMm{0.0}; ///< 0 = 500 cm, the PDF limit
    qreal m_marginMm{5.0};
    Unit m_unit{Unit::Cm};
};

#endif // OEXPORTROLLFORMATWIDGET_H
