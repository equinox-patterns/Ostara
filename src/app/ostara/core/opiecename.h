/************************************************************************
 **
 **  @file   opiecename.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/10/2026
 **
 **  @brief  Nommer les objets sans nom qu'une pièce emploie, avant de l'enregistrer.
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
#ifndef OPIECENAME_H
#define OPIECENAME_H

#include "opiecemark.h"

#include "../vtools/undocommands/vundocommand.h"

#include <QDomElement>
#include <QString>
#include <QStringList>
#include <QVector>

class VContainer;
class VPattern;

namespace OPieceName
{
/**
 * @brief Un objet sans nom qu'une pièce emploie : de quoi le nommer, et où on l'appelle.
 *
 * Un objet sans nom est un objet auquel personne n'a donné d'alias — une ligne que le conteneur ne connaît que
 * par la paire de points qu'elle joint, une courbe qui ne répond que par la route sur laquelle elle est bâtie.
 * Une telle pièce ne peut pas être enregistrée : le nom qu'elle emploie est composé par le patron, et rien ne le
 * suivrait dans un autre fichier.
 */
struct Blocker
{
    QString name{};              ///< le nom tel qu'il est écrit — `Y_Y1`, `R1_Eb` : ce que la fenêtre propose.
    OPieceMark::AliasOf of{};    ///< la nature de l'objet : `Ligne`, `Courbe`, `Arc`, `Arc elliptique`.
    QString slot{};              ///< le slot du schéma qui porte le nom, `parent/element/@attribute`.
    QDomElement carrier{};       ///< l'élément à écrire — vide pour une ligne, qui vit dans `<lineNames>`.
    QString pairKey{};           ///< la paire d'une ligne, vide pour une courbe.
    quint32 curveId{0};          ///< l'identifiant de la courbe, vide pour une ligne.
    QString formula{};           ///< la **première** formule qui l'appelle : le contexte, pour savoir comment le nommer.
};

/// Les objets sans nom qu'une pièce emploie, chez elle comme chez les pièces qui la lisent — un par objet, dans
/// l'ordre du document, avec la première formule qui l'appelle.
auto Blockers(VPattern *doc, VContainer *data, const QString &blockName) -> QVector<Blocker>;

/**
 * @brief Réparer les formules qui appelaient une courbe par le **nom composé** qui vient de recevoir un alias.
 *
 * L'outil écrit l'alias et répare les citations de son **ancien alias** — mais une courbe jamais nommée est citée
 * par son nom composé (`SplPath_R1_Eb`, `Curve_A11_A16__o2`), que l'outil ne connaît pas : sans cette passe, la
 * pièce qui la lit garde l'ancien jeton et ne trouve plus rien. Le document entier est parcouru, parce qu'une
 * dépendance vit dans une autre pièce.
 */
class NameCurves : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    /// @param renamed les paires « nom composé → alias », dans l'ordre où la fenêtre les a données.
    NameCurves(VPattern *doc, const QVector<QPair<QString, QString>> &renamed, QUndoCommand *parent = nullptr);
    ~NameCurves() override = default;

    void undo() override;
    void redo() override;

private:
    Q_DISABLE_COPY_MOVE(NameCurves) // NOLINT

    struct Field
    {
        QDomElement element{};
        QString attribute{};
        QString text{};
    };

    QVector<QPair<QString, QString>> m_renamed{};
    QVector<Field> m_fields{}; ///< ce que les formules disaient avant, pour les remettre.
    bool m_saved{false};
};

} // namespace OPieceName

#endif // OPIECENAME_H
