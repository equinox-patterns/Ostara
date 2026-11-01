/************************************************************************
 **
 **  @file   opieceimportdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  How a library piece enters a pattern: the name it takes, and the aliases it carries.
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
#ifndef OPIECEIMPORTDIALOG_H
#define OPIECEIMPORTDIALOG_H

#include "../core/opieceimport.h"
#include "../core/opieceinsert.h"

#include <QDialog>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Ui {
class OPieceImportDialog;
}

class QComboBox;
class QLabel;
class QTableWidget;

/**
 * @brief What a piece entering the pattern asks before it enters: the name it will wear, and every alias it carries.
 *
 * The aliases are the keys the container answers to by a head — `Spl_dos`, `SplPath_Em_R1`, `Line_Angle180` — and
 * they are shown **whether a formula cites them or not**: what is decided here is the name the piece will wear, not
 * what reads it. Each line comes prefilled with the proposal: the piece's own alias when the pattern answers to
 * none of its names, the same alias marked `__P<n>` when it does. A name that is empty, badly shaped, already
 * answered to by the pattern, or given to two lines at once keeps the accept button out of reach.
 *
 * The dialogue **writes nothing**: it hands back a name and a table of renamings, and the caller applies them in
 * one operation. A cancelled dialogue leaves the pattern exactly as it was.
 */
class OPieceImportDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    /**
     * @param proposal what the piece asks — its aliases, and the marks the pattern proposes.
     * @param blockNames the names the pattern's pieces already wear, which the instance name has to stay out of.
     * @param taken the keys the pattern's container answers to, which no alias may take the place of.
     * @param references the values the piece reads elsewhere that **several** blocks of the pattern can satisfy: one
     *        combo each, so that the piece is based on the instance the reader names. Empty when every dependency
     *        has one candidate or none — the second case being refused before the dialogue opens.
     */
    OPieceImportDialog(const OPieceInsert::Proposal &proposal, const QStringList &blockNames, const QSet<QString> &taken,
                       const QVector<OPieceImport::Reference> &references, QWidget *parent = nullptr);
    ~OPieceImportDialog() override;

    /// @brief The name the block will wear — the one the top bar will show.
    auto InstanceName() const -> QString;

    /// @brief The renamings to apply: every name the piece declares, with the aliases as they were left here.
    auto Table() const -> QHash<QString, QString>;

    /// @brief Le bloc choisi pour chaque valeur lue ailleurs : `jeton → nom du bloc`, vide quand rien n'a été demandé.
    auto ChosenReferences() const -> QHash<QString, QString>;

    /// @brief Les incréments que le patron définit autrement, renommés : `#ancien → #nouveau`.
    ///
    /// Vide quand rien ne se dispute : un incrément que les deux fichiers définissent de la même façon est un
    /// doublon, et il n'y a rien à demander.
    auto IncrementTable() const -> QHash<QString, QString>;

private slots:
    void CheckState();

private:
    Q_DISABLE_COPY_MOVE(OPieceImportDialog) // NOLINT

    void FillAliases();

    /// @brief Les incréments en conflit : ce que chaque fichier en dit, et le nom que celui de la pièce prendra.
    void FillIncrements();

    /// @brief Les valeurs lues ailleurs que plusieurs blocs peuvent satisfaire : un combo chacune, avant les alias.
    void FillReferences();

    /// @brief Le genre d'objet qui porte l'alias : `Ligne`, `Courbe`, `Arc`, `Arc elliptique`.
    static auto ObjectText(const OPieceMark::Alias &alias) -> QString;

    /// @brief Show @p text under a field, and take it away when there is nothing to say.
    static void SetProblem(QLabel *label, const QString &text);

    /// @brief What is wrong with @p typed, if anything: the shape a name of the schema has, and the keys taken.
    auto AliasProblem(const QString &typed) const -> QString;

    Ui::OPieceImportDialog *ui;
    OPieceInsert::Proposal m_proposal{};
    QStringList m_blockNames{};
    QSet<QString> m_taken{};
    QVector<OPieceImport::Reference> m_references{};
    QHash<QString, QComboBox *> m_referenceBoxes{};
    QTableWidget *m_incrementTable{nullptr};
    QLabel *m_incrementProblem{nullptr};
};

#endif // OPIECEIMPORTDIALOG_H
