/************************************************************************
 **
 **  @file   dialogseamallowance_paths.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Path/node list management for DialogSeamAllowance.
 **  @note   Extracted from dialogseamallowance.cpp
 **          as part of the Ostara refactoring.
 **
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#include "dialogseamallowance.h"
#include "dialogseamallowance_helpers.h"
#include "../../../tools/vtoolseamallowance.h"
#include "../../../undocommands/savepiecepathoptions.h"
#include "../../../undocommands/saveplacelabeloptions.h"
#include "../../../visualization/path/vistoolpiece.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "dialogpiecepath.h"
#include "dialogplacelabel.h"
#include "ui_tabpaths.h"
#include "ui_tabplacelabels.h"

#include <QMenu>
#include <QMessageBox>
#include <QScopeGuard>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ListChanged()
{
    if (not applyAllowed)
    {
        auto *visPath = qobject_cast<VisToolPiece *>(vis);
        SCASSERT(visPath != nullptr);
        visPath->SetPiece(CreatePiece());
        visPath->RefreshGeometry();
    }
    InitNodesList();
    InitPassmarksList();
    InitMirrorLine();
    CustomSAChanged(uiTabPaths->listWidgetCustomSA->currentRow());
    SetMoveControls();
    SetOptionControls();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::NodeChanged(int index)
{
    uiTabPaths->plainTextEditFormulaWidthBefore->setDisabled(true);
    uiTabPaths->toolButtonExprBefore->setDisabled(true);
    uiTabPaths->pushButtonDefBefore->setDisabled(true);

    uiTabPaths->plainTextEditFormulaWidthAfter->setDisabled(true);
    uiTabPaths->toolButtonExprAfter->setDisabled(true);
    uiTabPaths->pushButtonDefAfter->setDisabled(true);

    uiTabPaths->comboBoxAngle->setDisabled(true);

    const QSignalBlocker blocker(uiTabPaths->comboBoxAngle);

    if (index != -1)
    {
        const VPiece piece = CreatePiece();
        const int nodeIndex = piece.GetPath().indexOfNode(uiTabPaths->comboBoxNodes->currentData().toUInt());
        if (nodeIndex != -1)
        {
            const VPiecePath &path = piece.GetPath();
            const VPieceNode &node = path.at(nodeIndex);

            // Seam alowance before
            uiTabPaths->plainTextEditFormulaWidthBefore->setEnabled(true);
            uiTabPaths->toolButtonExprBefore->setEnabled(true);

            QString w1Formula = node.GetFormulaSABefore();
            EnableDefButton(uiTabPaths->pushButtonDefBefore, w1Formula);
            w1Formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                w1Formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            if (w1Formula.length() > 80) // increase height if needed.
            {
                this->DeployWidthBeforeFormulaTextEdit();
            }
            uiTabPaths->plainTextEditFormulaWidthBefore->setPlainText(w1Formula);
            MoveCursorToEnd(uiTabPaths->plainTextEditFormulaWidthBefore);

            // Seam alowance after
            uiTabPaths->plainTextEditFormulaWidthAfter->setEnabled(true);
            uiTabPaths->toolButtonExprAfter->setEnabled(true);

            QString w2Formula = node.GetFormulaSAAfter();
            EnableDefButton(uiTabPaths->pushButtonDefAfter, w2Formula);
            w2Formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                w2Formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            if (w2Formula.length() > 80) // increase height if needed.
            {
                this->DeployWidthAfterFormulaTextEdit();
            }
            uiTabPaths->plainTextEditFormulaWidthAfter->setPlainText(w2Formula);
            MoveCursorToEnd(uiTabPaths->plainTextEditFormulaWidthAfter);

            // Angle type
            uiTabPaths->comboBoxAngle->setEnabled(true);
            const int index = uiTabPaths->comboBoxAngle->findData(static_cast<unsigned char>(node.GetAngleType()));
            if (index != -1)
            {
                uiTabPaths->comboBoxAngle->setCurrentIndex(index);
            }
        }
    }
    else
    {
        uiTabPaths->plainTextEditFormulaWidthBefore->setPlainText(currentSeamAllowance);
        uiTabPaths->plainTextEditFormulaWidthAfter->setPlainText(currentSeamAllowance);
        uiTabPaths->comboBoxAngle->setCurrentIndex(-1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CSAStartPointChanged(int index)
{
    Q_UNUSED(index);

    const int row = uiTabPaths->listWidgetCustomSA->currentRow();
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetCustomSA->count())
    {
        return;
    }

    QListWidgetItem *rowItem = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(rowItem != nullptr)
    auto record = qvariant_cast<CustomSARecord>(rowItem->data(Qt::UserRole));
    record.startPoint = uiTabPaths->comboBoxStartPoint->currentData().toUInt();
    rowItem->setData(Qt::UserRole, QVariant::fromValue(record));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CSAEndPointChanged(int index)
{
    Q_UNUSED(index);

    const int row = uiTabPaths->listWidgetCustomSA->currentRow();
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetCustomSA->count())
    {
        return;
    }

    QListWidgetItem *rowItem = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(rowItem != nullptr)
    auto record = qvariant_cast<CustomSARecord>(rowItem->data(Qt::UserRole));
    record.endPoint = uiTabPaths->comboBoxEndPoint->currentData().toUInt();
    rowItem->setData(Qt::UserRole, QVariant::fromValue(record));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::MirrorLinePointChanged(int index)
{
    Q_UNUSED(index)

    flagMirrorLineIsValid = MirrorLineIsValid();
    uiTabPaths->checkBoxShowFullPiece->setEnabled(flagMirrorLineIsValid);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CSAIncludeTypeChanged(int index)
{
    Q_UNUSED(index);

    const int row = uiTabPaths->listWidgetCustomSA->currentRow();
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetCustomSA->count())
    {
        return;
    }

    QListWidgetItem *rowItem = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(rowItem != nullptr)
    auto record = qvariant_cast<CustomSARecord>(rowItem->data(Qt::UserRole));
    record.includeType = static_cast<PiecePathIncludeType>(uiTabPaths->comboBoxIncludeType->currentData().toUInt());
    rowItem->setData(Qt::UserRole, QVariant::fromValue(record));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::NodeAngleChanged(int index)
{
    const int i = uiTabPaths->comboBoxNodes->currentIndex();
    if (i != -1 && index != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPaths->comboBoxNodes->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetAngleType(static_cast<PieceNodeAngle>(uiTabPaths->comboBoxAngle->currentData().toUInt()));
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ReturnDefBefore()
{
    const QString def = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        currentSeamAllowance, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    uiTabPaths->plainTextEditFormulaWidthBefore->setPlainText(def);
    if (auto *button = qobject_cast<QPushButton *>(sender()))
    {
        button->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ReturnDefAfter()
{
    const QString def = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        currentSeamAllowance, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    uiTabPaths->plainTextEditFormulaWidthAfter->setPlainText(def);
    if (auto *button = qobject_cast<QPushButton *>(sender()))
    {
        button->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CustomSAChanged(int row)
{
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetCustomSA->count())
    {
        {
            const QSignalBlocker blocker(uiTabPaths->comboBoxStartPoint);
            uiTabPaths->comboBoxStartPoint->clear();
        }

        {
            const QSignalBlocker blocker(uiTabPaths->comboBoxEndPoint);
            uiTabPaths->comboBoxEndPoint->clear();
        }

        const QSignalBlocker blocker(uiTabPaths->comboBoxIncludeType);
        uiTabPaths->comboBoxIncludeType->clear();
        return;
    }

    const QListWidgetItem *item = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(item != nullptr);
    const auto record = qvariant_cast<CustomSARecord>(item->data(Qt::UserRole));

    {
        const QSignalBlocker blocker(uiTabPaths->comboBoxStartPoint);
        InitCSAPoint(uiTabPaths->comboBoxStartPoint);
        {
            const int index = uiTabPaths->comboBoxStartPoint->findData(record.startPoint);
            if (index != -1)
            {
                uiTabPaths->comboBoxStartPoint->setCurrentIndex(index);
            }
        }
    }

    {
        const QSignalBlocker blocker(uiTabPaths->comboBoxEndPoint);
        InitCSAPoint(uiTabPaths->comboBoxEndPoint);
        {
            const int index = uiTabPaths->comboBoxEndPoint->findData(record.endPoint);
            if (index != -1)
            {
                uiTabPaths->comboBoxEndPoint->setCurrentIndex(index);
            }
        }
    }

    const QSignalBlocker blocker(uiTabPaths->comboBoxIncludeType);
    InitSAIncludeType();
    {
        const int index = uiTabPaths->comboBoxIncludeType->findData(static_cast<unsigned char>(record.includeType));
        if (index != -1)
        {
            uiTabPaths->comboBoxIncludeType->setCurrentIndex(index);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PathDialogClosed(int result)
{
    if (result == QDialog::Accepted)
    {
        SCASSERT(not m_dialog.isNull());
        auto *dialogTool = qobject_cast<DialogPiecePath *>(m_dialog.data());
        SCASSERT(dialogTool != nullptr);
        try
        {
            VPiecePath const currentPath = CurrentPath(dialogTool->GetToolId());
            VPiecePath const newPath = dialogTool->GetPiecePath();
            m_newPaths.insert(dialogTool->GetToolId(), newPath);

            QPointer<VUndoCommand> const saveCommand = new SavePiecePathOptions(
                toolId, currentPath, newPath, VAbstractValApplication::VApp()->getCurrentDocument(),
                dialogTool->GetToolId());
            m_undoStack.append(saveCommand);
            UpdateCurrentCustomSARecord();
            UpdateCurrentInternalPathRecord();
        }
        catch (const VExceptionBadId &e)
        {
            qCritical("%s\n\n%s\n\n%s", qUtf8Printable(tr("Error. Can't save piece path.")),
                      qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        }
    }
    delete m_dialog.data();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PlaceLabelDialogClosed(int result)
{
    if (result == QDialog::Accepted)
    {
        SCASSERT(not m_dialog.isNull());
        auto *dialogTool = qobject_cast<DialogPlaceLabel *>(m_dialog.data());
        SCASSERT(dialogTool != nullptr);
        try
        {
            VPlaceLabelItem const currentLabel = CurrentPlaceLabel(dialogTool->GetToolId());

            const QHash<QString, QSharedPointer<VInternalVariable>> *vars = data.DataVariables();

            const qreal w = qAbs(Visualization::FindLengthFromUser(dialogTool->GetWidth(), vars, false));
            const qreal h = qAbs(Visualization::FindLengthFromUser(dialogTool->GetHeight(), vars, false));
            const qreal a = Visualization::FindValFromUser(dialogTool->GetAngle(), vars, false);
            const qreal v = Visualization::FindValFromUser(dialogTool->GetFormulaVisible(), vars, false);
            qDebug() << w << h << a << v;

            auto newLabel = VPlaceLabelItem();
            newLabel.setName(currentLabel.name());
            newLabel.setX(currentLabel.x());
            newLabel.setY(currentLabel.y());
            newLabel.setMx(currentLabel.mx());
            newLabel.setMy(currentLabel.my());
            newLabel.SetWidth(w, dialogTool->GetWidth());
            newLabel.SetHeight(h, dialogTool->GetHeight());
            newLabel.SetAngle(a, dialogTool->GetAngle());
            newLabel.SetVisibilityTrigger(v, dialogTool->GetFormulaVisible());
            newLabel.SetLabelType(dialogTool->GetLabelType());
            newLabel.SetCenterPoint(currentLabel.GetCenterPoint());
            newLabel.SetCorrectionAngle(currentLabel.GetCorrectionAngle());
            newLabel.SetNotMirrored(dialogTool->IsNotMirrored());

            m_newPlaceLabels.insert(dialogTool->GetToolId(), newLabel);

            QPointer<VUndoCommand> const saveCommand = new SavePlaceLabelOptions(
                toolId, currentLabel, newLabel, VAbstractValApplication::VApp()->getCurrentDocument(),
                dialogTool->GetToolId());
            m_undoStack.append(saveCommand);
            UpdateCurrentPlaceLabelRecords();
        }
        catch (const VExceptionBadId &e)
        {
            qCritical("%s\n\n%s\n\n%s", qUtf8Printable(tr("Error. Can't save piece path.")),
                      qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        }
    }
    delete m_dialog.data();
}

