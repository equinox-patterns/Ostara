/************************************************************************
 **
 **  @file   dialoginsertnode.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   21 3, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#include "dialoginsertnode.h"
#include "themeDef.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/tools/toolsdef.h"
#include "../vtools/visualization/path/vistoolinsertnode.h"
#include "../../../../vmisc/FactoryWidgets.h"
#include "ui_dialoginsertnode.h"

//---------------------------------------------------------------------------------------------------------------------
DialogInsertNode::DialogInsertNode(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogInsertNode)
{
    ui->setupUi(this);
    InitOkCancel(ui);

    // The piece combo comes from the WidgetFactory: styled dropdown (QListView),
    // so the global QSS is respected (the native macOS popup ignores it).
    ApplyComboBoxView(ui->comboBoxPiece);

    CheckPieces();

    connect(ui->comboBoxPiece, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { CheckPieces(); });

    vis = new VisToolInsertNode(&this->data);
}

//---------------------------------------------------------------------------------------------------------------------
DialogInsertNode::~DialogInsertNode()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::SetPiecesList(const QVector<quint32> &list)
{
    FillComboBoxPiecesList(ui->comboBoxPiece, list);

    if (list.isEmpty())
    {
        qWarning() << tr("The list of pieces is empty. Please, first create at least one piece for current pattern "
                         "piece.");
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogInsertNode::GetPieceId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxPiece);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::SetPieceId(quint32 id)
{
    if (ui->comboBoxPiece->count() <= 0)
    {
        ui->comboBoxPiece->addItem(data.GetPiece(id).GetName(), id);
    }
    else
    {
        const qint32 index = ui->comboBoxPiece->findData(id);
        if (index != -1)
        {
            ui->comboBoxPiece->setCurrentIndex(index);
        }
        else
        {
            ui->comboBoxPiece->setCurrentIndex(0);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogInsertNode::GetNodes() const -> QVector<VPieceNode>
{
    return m_nodes;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::ShowDialog(bool click)
{
    if (not click)
    {
        if (m_nodes.isEmpty())
        {
            return;
        }

        // No object list: the selection is already visible on the canvas (VisToolInsertNode).
        CheckPieces();

        prepare = true;
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::CheckDependencyTreeComplete()
{
    // does nothing
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::ChosenObject(quint32 id, const SceneObject &type)
{
    Q_UNUSED(type)

    if (prepare)
    {
        return;
    }

    if (auto nodeIterator = std::find_if(m_nodes.begin(),
                                         m_nodes.end(),
                                         [id](const VPieceNode &node) -> bool { return node.GetId() == id; });
        nodeIterator == m_nodes.cend())
    {
        GOType objectType = GOType::Unknown;
        try
        {
            objectType = data.GetGObject(id)->getType();
        }
        catch (const VExceptionBadId &)
        {
            qDebug() << "Cannot find an object with id" << id;
            return;
        }

        VPieceNode node;
        switch (objectType)
        {
            case GOType::Arc:
                node = VPieceNode(id, Tool::NodeArc);
                break;
            case GOType::EllipticalArc:
                node = VPieceNode(id, Tool::NodeElArc);
                break;
            case GOType::Point:
                node = VPieceNode(id, Tool::NodePoint);
                break;
            case GOType::Spline:
            case GOType::CubicBezier:
                node = VPieceNode(id, Tool::NodeSpline);
                break;
            case GOType::SplinePath:
            case GOType::CubicBezierPath:
                node = VPieceNode(id, Tool::NodeSplinePath);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
            default:
                qDebug() << "Got unexpected object type. Ignore.";
                return;
        }

        node.SetExcluded(true);
        m_nodes.append(node);
    }
    else
    {
        m_nodes.erase(nodeIterator);
    }

    auto *operation = qobject_cast<VisToolInsertNode *>(vis);
    SCASSERT(operation != nullptr)
    operation->SetNodes(NodesToObjects(m_nodes));
    if (!VAbstractValApplication::VApp()->getCurrentScene()->items().contains(operation))
    {
        operation->VisualMode(NULL_ID);
    }
    else
    {
        operation->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogInsertNode::CheckPieces()
{
    QColor color;
    if (ui->comboBoxPiece->count() <= 0 || ui->comboBoxPiece->currentIndex() == -1)
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    ChangeColor(ui->labelPiece, color);
    CheckState();
}

