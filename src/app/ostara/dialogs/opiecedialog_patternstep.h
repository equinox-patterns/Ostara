/************************************************************************
 **
 **  @file   opiecedialog_patternstep.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Step 1 of the piece tool dialog: pattern-level information
 **          (cartouche, pattern name, client, collection, fabric info and
 **          piece creation seeds). Shown once, for the first piece only.
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

#ifndef OPIECEDIALOG_PATTERNSTEP_H
#define OPIECEDIALOG_PATTERNSTEP_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class VAbstractPattern;

/**
 * @brief Pattern-level fields of the first-piece dialog step.
 *
 * The widget only collects values; the host dialog commits them to the
 * document on Next (Skip commits nothing).
 */
class OPiecePatternStep final : public QWidget
{
    Q_OBJECT

public:
    explicit OPiecePatternStep(VAbstractPattern *doc, QWidget *parent = nullptr);

    auto GetCartouche() const -> QString;
    auto GetPatternName() const -> QString;
    auto GetClient() const -> QString;
    auto GetCollection() const -> QString;
    auto GetFabricInfo() const -> QString;
    auto IsActivateSA() const -> bool;
    auto IsHideMainPath() const -> bool;

private:
    VAbstractPattern *m_doc{nullptr};
    QComboBox *m_cartoucheCombo{nullptr};
    QLineEdit *m_patternNameEdit{nullptr};
    QComboBox *m_clientCombo{nullptr};
    QComboBox *m_collectionCombo{nullptr};
    QPlainTextEdit *m_fabricInfoEdit{nullptr};
    QCheckBox *m_activateSACheck{nullptr};
    QCheckBox *m_hideMainPathCheck{nullptr};
};

#endif // OPIECEDIALOG_PATTERNSTEP_H
