/************************************************************************
 **
 **  @file   dialoginsertnode.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   21 3, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 28/08/2026.
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

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#ifndef DIALOGINSERTNODE_H
#define DIALOGINSERTNODE_H

#include "../dialogtool.h"
#include "../vpatterndb/vpiecenode.h"

namespace Ui
{
class DialogInsertNode;
}

class DialogInsertNode final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogInsertNode(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogInsertNode() override;

    void SetPiecesList(const QVector<quint32> &list) override;

    auto GetPieceId() const -> quint32;
    void SetPieceId(quint32 id);

    auto GetNodes() const -> QVector<VPieceNode>;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(DialogInsertNode) // NOLINT
    Ui::DialogInsertNode *ui;

    QVector<VPieceNode> m_nodes{};
    bool m_flagError{false};

    void CheckPieces();
};

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogInsertNode::IsValid() const -> bool
{
    // The dialog only opens with a non-empty node selection (ShowDialog), so
    // the piece selection is the only remaining gate.
    return m_flagError;
}

#endif // DIALOGINSERTNODE_H
