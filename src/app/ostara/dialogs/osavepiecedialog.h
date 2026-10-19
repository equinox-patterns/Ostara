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
 * @brief Asks for the name a piece takes in the library, and the folder it is saved in.
 *
 * The field comes prefilled with the piece's own name and stays editable: the name given here is the one
 * the library will call the piece, and the one its file will be named after. Under it, the tree of the library's
 * folders — the same one its window shows — chooses where the file is written, and a folder can be made there
 * without leaving the dialogue. The dialogue answers and writes nothing — the caller writes the file, once, after
 * the last agreement — and a refusal costs nothing, which is what lets the caller ask again with the name to
 * change.
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
    /**
     * @param libraryDirectory the library's own folder — the root of the tree — given, never guessed, so the
     *                         dialogue can be pointed at the pieces of a test like the window is.
     */
    explicit OSavePieceDialog(const QString &pieceName, const QString &libraryDirectory,
                              const QVector<OPieceName::Blocker> &blockers = {}, const QSet<QString> &taken = {},
                              QWidget *parent = nullptr);
    ~OSavePieceDialog() override;

    /// @brief The name given, meaningful once accepted.
    auto name() const -> QString;

    /// @brief The file the piece will be written to: the name in the field, in the folder the tree points at.
    ///        Meaningful once accepted — a name holding a separator never gets there.
    auto LibraryPath() const -> QString;

    /// @brief What the reader wants to remember about the piece — written **on the block**, so the `.opiece` the
    /// caller is about to write carries it, and the pattern's own piece carries the same text from then on.
    auto notes() const -> QString;

    /// @brief Shows @p notes in the field: the note the piece already has, so saving again does not lose it.
    void SetNotes(const QString &notes);

    /// @brief Les noms donnés aux objets sans nom, dans l'ordre où la fenêtre les montre.
    auto Names() const -> QStringList;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void CheckState();

    /// @brief Fills the tree with the library's folders, the library's own folder first, and puts the choice back.
    void PopulateTree();

    /// @brief A folder made in the dialogue: it becomes the destination, since that is why it was made.
    void OnCreateFolder();

    /// @brief Says in plain words where the file will land: a tree and a name do not make a path by themselves.
    void ShowDestination();

private:
    Q_DISABLE_COPY_MOVE(OSavePieceDialog) // NOLINT

    /// @brief The folder the tree points at — the library's own folder when it points at nothing.
    auto TargetFolder() const -> QString;

    /// @brief Montrer les objets sans nom : la nature, le nom proposé, et la formule qui les appelle.
    void FillBlockers();

    /// @brief La nature d'un objet, dite comme l'utilisateur la lit : `Ligne`, `Courbe`, `Arc`, `Arc elliptique`.
    auto ObjectText(const OPieceMark::AliasOf &of) const -> QString;

    /// @brief Ce qui ne va pas dans @p typed, s'il y a quelque chose : vide, mal formé, ou donné deux fois.
    auto NameProblem(const QString &typed, const QStringList &others) const -> QString;

    Ui::OSavePieceDialog *ui;
    QString m_library{};
    QVector<OPieceName::Blocker> m_blockers{};
    QSet<QString> m_taken{};
};

#endif // OSAVEPIECEDIALOG_H
