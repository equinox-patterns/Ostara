/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 10/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef DELETEPIECE_H
#define DELETEPIECE_H

#include <QDomElement>
#include <QSet>
#include <QVector>
#include <QtGlobal>

#include "../tools/vtoolseamallowance.h"
#include "../vpatterndb/vpiece.h"
#include "vundocommand.h"

class DeletePiece : public VUndoCommand
{
    Q_OBJECT // NOLINT
public:
    DeletePiece(VAbstractPattern *doc, quint32 id, const VContainer &data, VMainGraphicsScene *scene,
                QUndoCommand *parent = nullptr);
    ~DeletePiece() override;

    void undo() override;
    void redo() override;

private:
    Q_DISABLE_COPY_MOVE(DeletePiece) // NOLINT

    /// The objects of `<modeling>` a piece draws through and the paths it owns, as the file names them.
    struct PieceModelingRefs
    {
        QSet<vidtype> nodes{};          ///< ids of the node copies named by the piece's nodes
        QVector<QDomElement> paths{};   ///< the <path> elements the piece's <iPaths> name
    };

    /// One element of `<modeling>` that goes away with the piece.
    struct OwnedModelingElement
    {
        QDomElement parent{};  ///< the `<modeling>` it was read from
        QDomElement element{}; ///< its clone, the copy that survives the removal and comes back on undo
    };

    auto ModelingRefsOf(const QDomElement &detail) const -> PieceModelingRefs;
    void CollectOwnedModeling(const QDomElement &detail);
    void TakeOwnedModeling();
    void RestoreOwnedModeling();

    QDomNode m_parentNode{};
    quint32 m_siblingId{};
    VPiece m_detail;
    VContainer m_data;
    VMainGraphicsScene *m_scene;
    QPointer<VToolSeamAllowance> m_tool{};
    VToolRecord m_record;
    int m_indexPatternBlock{-1};
    QVector<OwnedModelingElement> m_ownedModeling{};
};

#endif // DELETEPIECE_H
