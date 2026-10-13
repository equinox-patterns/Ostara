/************************************************************************
 **
 **  @file   osavepiecedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The name a piece will wear in the library, asked before anything is written.
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
#ifndef OSAVEPIECEDIALOG_H
#define OSAVEPIECEDIALOG_H

#include "../core/opiecename.h"

#include <QDialog>
#include <QString>
#include <QSet>
#include <QStringList>
#include <QVector>

namespace Ui {
class OSavePieceDialog;
}

class QEvent;

/**
 * @brief Asks for the name a piece takes in the library.
 *
 * The field comes prefilled with the piece's own name and stays editable: the name given here is the one
 * the library will call the piece, and the one its file will be named after. The dialogue answers and
 * writes nothing — the caller writes the file, once, after the last agreement — and a refusal costs
 * nothing, which is what lets the caller ask again with the name to change.
 */
class OSavePieceDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /**
     * @param blockers les objets sans nom que la pièce emploie : la fenêtre les montre, et le nom donné à chacun
     *                 sera écrit dans le patron avant l'enregistrement. Vide, la fenêtre est celle d'avant.
     */
    /**
     * @param taken les noms que le patron déclare déjà — noms et alias de tous ses objets : un alias qui en
     *              prend un n'est pas libre, et la fenêtre le dit **avant** que l'outil ne refuse.
     */
    explicit OSavePieceDialog(const QString &pieceName, const QVector<OPieceName::Blocker> &blockers = {},
                              const QSet<QString> &taken = {}, QWidget *parent = nullptr);
    ~OSavePieceDialog() override;

    /// @brief The name given, meaningful once accepted.
    auto name() const -> QString;

    /// @brief Les noms donnés aux objets sans nom, dans l'ordre où la fenêtre les montre.
    auto Names() const -> QStringList;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void CheckState();

private:
    Q_DISABLE_COPY_MOVE(OSavePieceDialog) // NOLINT

    /// @brief Montrer les objets sans nom : la nature, le nom proposé, et la formule qui les appelle.
    void FillBlockers();

    /// @brief La nature d'un objet, dite comme l'utilisateur la lit : `Ligne`, `Courbe`, `Arc`, `Arc elliptique`.
    auto ObjectText(const OPieceMark::AliasOf &of) const -> QString;

    /// @brief Ce qui ne va pas dans @p typed, s'il y a quelque chose : vide, mal formé, ou donné deux fois.
    auto NameProblem(const QString &typed, const QStringList &others) const -> QString;

    Ui::OSavePieceDialog *ui;
    QVector<OPieceName::Blocker> m_blockers{};
    QSet<QString> m_taken{};
};

#endif // OSAVEPIECEDIALOG_H
