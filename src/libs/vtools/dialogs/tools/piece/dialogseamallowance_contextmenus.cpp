/************************************************************************
 **
 **  @file   dialogseamallowance_contextmenus.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Context menus for DialogSeamAllowance.
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
#include "ui_tabgrainline.h"
#include "ui_tabpassmarks.h"
#include "ui_tabpaths.h"
#include "ui_tabpins.h"
#include "ui_tabplacelabels.h"
#include "../../../undocommands/saveplacelabeloptions.h"
#include "../../../visualization/path/vistoolpiece.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vmisc/def.h"
#include "vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vwidgets/fancytabbar/fancytabbar.h"
#include "dialogpiecepath.h"
#include "dialogplacelabel.h"

#include <QMenu>
#include <QScopedPointer>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowMainPathContextMenu(const QPoint &pos)
{
    const int row = uiTabPaths->listWidgetMainPath->currentRow();
    if (uiTabPaths->listWidgetMainPath->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetMainPath->count())
    {
        return;
    }

    QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
    SCASSERT(rowItem != nullptr)
    auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

    QMenu menu;
    QHash<int, QAction *> const contextMenu = InitMainPathContextMenu(&menu, rowNode);

    QAction *selectedAction = menu.exec(uiTabPaths->listWidgetMainPath->viewport()->mapToGlobal(pos));
    auto selectedOption = static_cast<MainPathContextMenuOption>(
        contextMenu.key(selectedAction, static_cast<int>(MainPathContextMenuOption::NoSelection)));

    auto SelectPassmarkLineType = [this, &rowNode, rowItem](PassmarkLineType type)
    {
        rowNode.SetPassmark(true);
        rowNode.SetPassmarkLineType(type);
        rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        rowItem->setText(GetNodeName(&data, rowNode, true));
    };

    Q_STATIC_ASSERT_X(static_cast<int>(MainPathContextMenuOption::LAST_ONE_DO_NOT_USE) == 17,
                      "Not all options were handled.");

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (selectedOption)
    {
        case MainPathContextMenuOption::NoSelection:
            return;
        case MainPathContextMenuOption::Reverse:
            rowNode.SetReverse(not rowNode.GetReverse());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));
            break;
        case MainPathContextMenuOption::NonePassmark:
            rowNode.SetPassmark(false);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));
            break;
        case MainPathContextMenuOption::OneLine:
            SelectPassmarkLineType(PassmarkLineType::OneLine);
            break;
        case MainPathContextMenuOption::TwoLines:
            SelectPassmarkLineType(PassmarkLineType::TwoLines);
            break;
        case MainPathContextMenuOption::ThreeLines:
            SelectPassmarkLineType(PassmarkLineType::ThreeLines);
            break;
        case MainPathContextMenuOption::TMark:
            SelectPassmarkLineType(PassmarkLineType::TMark);
            break;
        case MainPathContextMenuOption::ExternalVMark:
            SelectPassmarkLineType(PassmarkLineType::ExternalVMark);
            break;
        case MainPathContextMenuOption::InternalVMark:
            SelectPassmarkLineType(PassmarkLineType::InternalVMark);
            break;
        case MainPathContextMenuOption::UMark:
            SelectPassmarkLineType(PassmarkLineType::UMark);
            break;
        case MainPathContextMenuOption::BoxMark:
            SelectPassmarkLineType(PassmarkLineType::BoxMark);
            break;
        case MainPathContextMenuOption::CheckMark:
            SelectPassmarkLineType(PassmarkLineType::CheckMark);
            break;
        case MainPathContextMenuOption::Uniqueness:
            rowNode.SetCheckUniqueness(not rowNode.IsCheckUniqueness());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));
            break;
        case MainPathContextMenuOption::TurnPoint:
            rowNode.SetTurnPoint(not rowNode.IsTurnPoint());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));
            break;
        case MainPathContextMenuOption::Excluded:
            rowNode.SetExcluded(not rowNode.IsExcluded());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));
            rowItem->setFont(NodeFont(rowItem->font(), rowNode.IsExcluded()));
            break;
        case MainPathContextMenuOption::DuplicateNode:
        {
            VPieceNode duplicate = rowNode;
            duplicate.SetExcluded(true);

            auto *item = new QListWidgetItem(GetNodeName(&data, duplicate, true));
            item->setFont(NodeFont(item->font(), true));
            item->setData(Qt::UserRole, QVariant::fromValue(duplicate));
            uiTabPaths->listWidgetMainPath->addItem(item);
            break;
        }
        case MainPathContextMenuOption::Delete:
            delete uiTabPaths->listWidgetMainPath->item(row);
            break;
        case MainPathContextMenuOption::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
    };

    QT_WARNING_POP

    ValidObjects(MainPathIsValid());
    ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowCustomSAContextMenu(const QPoint &pos)
{
    const int row = uiTabPaths->listWidgetCustomSA->currentRow();
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1 || row >= uiTabPaths->listWidgetCustomSA->count())
    {
        return;
    }

    QScopedPointer<QMenu> const menu(new QMenu());
    QAction  const*actionOption = menu->addAction(FromTheme(VThemeIcon::PreferencesOther), tr("Options"));

    QListWidgetItem *rowItem = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(rowItem != nullptr)
    auto record = qvariant_cast<CustomSARecord>(rowItem->data(Qt::UserRole));

    QAction *actionReverse = menu->addAction(tr("Reverse"));
    actionReverse->setCheckable(true);
    actionReverse->setChecked(record.reverse);

    QAction  const*actionDelete = menu->addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));

    QAction  const*selectedAction = menu->exec(uiTabPaths->listWidgetCustomSA->viewport()->mapToGlobal(pos));
    if (selectedAction == actionDelete)
    {
        delete uiTabPaths->listWidgetCustomSA->item(row);
    }
    else if (selectedAction == actionReverse)
    {
        record.reverse = not record.reverse;
        rowItem->setData(Qt::UserRole, QVariant::fromValue(record));
        rowItem->setText(GetPathName(record.path, record.reverse));
    }
    else if (selectedAction == actionOption)
    {
        auto *dialog = new DialogPiecePath(&data, m_doc, record.path, this);
        dialog->SetPiecePath(CurrentPath(record.path));
        dialog->SetPieceId(toolId);
        if (record.includeType == PiecePathIncludeType::AsMainPath)
        {
            dialog->SetFormulaSAWidth(GetFormulaSAWidth());
        }
        dialog->HideVisibilityTrigger();
        dialog->EnbleShowMode(true);
        m_dialog = dialog;
        m_dialog->setModal(true);
        connect(m_dialog.data(), &DialogTool::DialogClosed, this, &DialogSeamAllowance::PathDialogClosed);
        m_dialog->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowInternalPathsContextMenu(const QPoint &pos)
{
    const int row = uiTabPaths->listWidgetInternalPaths->currentRow();
    if (uiTabPaths->listWidgetInternalPaths->count() == 0 || row == -1 ||
        row >= uiTabPaths->listWidgetInternalPaths->count())
    {
        return;
    }

    QScopedPointer<QMenu> const menu(new QMenu());
    QAction  const*actionOption = menu->addAction(FromTheme(VThemeIcon::PreferencesOther), tr("Options"));
    QAction  const*actionDelete = menu->addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));

    QAction  const*selectedAction = menu->exec(uiTabPaths->listWidgetInternalPaths->viewport()->mapToGlobal(pos));
    if (selectedAction == actionDelete)
    {
        delete uiTabPaths->listWidgetInternalPaths->item(row);
    }
    else if (selectedAction == actionOption)
    {
        QListWidgetItem  const*rowItem = uiTabPaths->listWidgetInternalPaths->item(row);
        SCASSERT(rowItem != nullptr)
        const auto pathId = qvariant_cast<quint32>(rowItem->data(Qt::UserRole));

        auto *dialog = new DialogPiecePath(&data, m_doc, pathId, this);
        dialog->SetPiecePath(CurrentPath(pathId));
        dialog->SetPieceId(toolId);
        dialog->EnbleShowMode(true);
        m_dialog = dialog;
        m_dialog->setModal(true);
        connect(m_dialog.data(), &DialogTool::DialogClosed, this, &DialogSeamAllowance::PathDialogClosed);
        m_dialog->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowPinsContextMenu(const QPoint &pos)
{
    const int row = uiTabPins->listWidgetPins->currentRow();
    if (uiTabPins->listWidgetPins->count() == 0 || row == -1 || row >= uiTabPins->listWidgetPins->count())
    {
        return;
    }

    QScopedPointer<QMenu> const menu(new QMenu());
    QAction  const*actionDelete = menu->addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));

    QAction  const*selectedAction = menu->exec(uiTabPins->listWidgetPins->viewport()->mapToGlobal(pos));
    if (selectedAction == actionDelete)
    {
        delete uiTabPins->listWidgetPins->item(row);
        FancyTabChanged(m_ftb->CurrentIndex());
        InitAllPinComboboxes();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowPlaceLabelsContextMenu(const QPoint &pos)
{
    const int row = uiTabPlaceLabels->listWidgetPlaceLabels->currentRow();
    if (uiTabPlaceLabels->listWidgetPlaceLabels->count() == 0 || row == -1 ||
        row >= uiTabPlaceLabels->listWidgetPlaceLabels->count())
    {
        return;
    }

    QListWidgetItem  const*rowItem = uiTabPlaceLabels->listWidgetPlaceLabels->item(row);
    SCASSERT(rowItem != nullptr)
    const auto labelId = qvariant_cast<quint32>(rowItem->data(Qt::UserRole));
    VPlaceLabelItem currentLabel = CurrentPlaceLabel(labelId);

    QScopedPointer<QMenu> menu(new QMenu());

    auto InitAction = [&currentLabel, &menu](const QString &text, PlaceLabelType type)
    {
        QAction *action = menu->addAction(text);
        action->setCheckable(true);
        action->setChecked(currentLabel.GetLabelType() == type);
        return action;
    };

    auto SaveType = [this, &currentLabel, labelId](PlaceLabelType type)
    {
        auto newLabel = VPlaceLabelItem(currentLabel);
        newLabel.SetLabelType(type);
        m_newPlaceLabels.insert(labelId, newLabel);

        QPointer<VUndoCommand> const saveCommand = new SavePlaceLabelOptions(
            toolId, currentLabel, newLabel, VAbstractValApplication::VApp()->getCurrentDocument(),
            labelId);
        m_undoStack.append(saveCommand);
        UpdateCurrentPlaceLabelRecords();
    };

    QAction  const*actionOption = menu->addAction(FromTheme(VThemeIcon::PreferencesOther), tr("Options"));
    menu->addSeparator();
    QAction  const*actionSegment = InitAction(tr("Segment"), PlaceLabelType::Segment);
    QAction  const*actionRectangle = InitAction(tr("Rectangle"), PlaceLabelType::Rectangle);
    QAction  const*actionCross = InitAction(tr("Cross"), PlaceLabelType::Cross);
    QAction  const*actionTshaped = InitAction(tr("T-shaped"), PlaceLabelType::Tshaped);
    QAction  const*actionDoubletree = InitAction(tr("Doubletree"), PlaceLabelType::Doubletree);
    QAction  const*actionCorner = InitAction(tr("Corner"), PlaceLabelType::Corner);
    QAction  const*actionTriangle = InitAction(tr("Triangle"), PlaceLabelType::Triangle);
    QAction  const*actionHshaped = InitAction(tr("H-shaped"), PlaceLabelType::Hshaped);
    QAction  const*actionButton = InitAction(tr("Button"), PlaceLabelType::Button);
    QAction  const*actionCircle = InitAction(tr("Circle"), PlaceLabelType::Circle);
    menu->addSeparator();
    QAction  const*actionDelete = menu->addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));

    QAction  const*selectedAction = menu->exec(uiTabPlaceLabels->listWidgetPlaceLabels->viewport()->mapToGlobal(pos));
    if (selectedAction == actionDelete)
    {
        delete uiTabPlaceLabels->listWidgetPlaceLabels->item(row);
        FancyTabChanged(m_ftb->CurrentIndex());
    }
    else if (selectedAction == actionOption)
    {
        auto *dialog = new DialogPlaceLabel(&data, m_doc, labelId, this);
        dialog->EnbleShowMode(true);
        dialog->SetLabelType(currentLabel.GetLabelType());
        dialog->SetWidth(currentLabel.GetWidthFormula());
        dialog->SetHeight(currentLabel.GetHeightFormula());
        dialog->SetAngle(currentLabel.GetAngleFormula());
        dialog->SetFormulaVisible(currentLabel.GetVisibilityTrigger());
        dialog->SetPieceId(toolId);
        dialog->SetCenterPoint(currentLabel.GetCenterPoint());
        dialog->SetNotMirrored(currentLabel.IsNotMirrored());
        m_dialog = dialog;
        m_dialog->setModal(true);
        connect(m_dialog.data(), &DialogTool::DialogClosed, this, &DialogSeamAllowance::PlaceLabelDialogClosed);
        m_dialog->show();
    }
    else if (selectedAction == actionSegment)
    {
        SaveType(PlaceLabelType::Segment);
    }
    else if (selectedAction == actionRectangle)
    {
        SaveType(PlaceLabelType::Rectangle);
    }
    else if (selectedAction == actionCross)
    {
        SaveType(PlaceLabelType::Cross);
    }
    else if (selectedAction == actionTshaped)
    {
        SaveType(PlaceLabelType::Tshaped);
    }
    else if (selectedAction == actionDoubletree)
    {
        SaveType(PlaceLabelType::Doubletree);
    }
    else if (selectedAction == actionCorner)
    {
        SaveType(PlaceLabelType::Corner);
    }
    else if (selectedAction == actionTriangle)
    {
        SaveType(PlaceLabelType::Triangle);
    }
    else if (selectedAction == actionHshaped)
    {
        SaveType(PlaceLabelType::Hshaped);
    }
    else if (selectedAction == actionButton)
    {
        SaveType(PlaceLabelType::Button);
    }
    else if (selectedAction == actionCircle)
    {
        SaveType(PlaceLabelType::Circle);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitIcons()
{
    const auto resource = QStringLiteral("icon");

    const auto fxIcon = QStringLiteral("24x24/fx.png");
    uiTabGrainline->pushButtonRot->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabGrainline->pushButtonLen->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPassmarks->toolButtonExprLength->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPassmarks->toolButtonExprWidth->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPassmarks->toolButtonExprAngle->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPassmarks->toolButtonExprPassmarkVisible->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPaths->toolButtonExprWidth->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPaths->toolButtonExprBefore->setIcon(VTheme::GetIconResource(resource, fxIcon));
    uiTabPaths->toolButtonExprAfter->setIcon(VTheme::GetIconResource(resource, fxIcon));

    const auto equalIcon = QStringLiteral("24x24/equal.png");
    uiTabGrainline->labelEqual->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabGrainline->labelEqual_2->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPaths->label_2->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPaths->label_6->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPaths->label_10->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPassmarks->label_3->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPassmarks->label_4->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPassmarks->label_5->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));
    uiTabPassmarks->labelEqual_6->setPixmap(VTheme::GetPixmapResource(resource, equalIcon));

    uiTabPaths->label_3->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("32x32/clockwise.png")));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::InitMainPathContextMenu(QMenu *menu, const VPieceNode &rowNode) const
    -> QHash<int, QAction *>
{
    SCASSERT(menu != nullptr)

    QHash<int, QAction *> contextMenu;

    if (rowNode.GetTypeTool() != Tool::NodePoint)
    {
        QAction *actionReverse = menu->addAction(FromTheme(VThemeIcon::ListRemove),
                                                 QApplication::translate("DialogSeamAllowance", "Reverse"));
        actionReverse->setCheckable(true);
        actionReverse->setChecked(rowNode.GetReverse());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Reverse), actionReverse);

        QAction *actionDuplicate = menu->addAction(FromTheme(VThemeIcon::DuplicateNode),
                                                  QApplication::translate("DialogSeamAllowance", "Duplicate"));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::DuplicateNode), actionDuplicate);
    }
    else
    {
        if (applyAllowed)
        {
            QMenu *passmarkSubmenu = menu->addMenu(FromTheme(VThemeIcon::AddPlacemark),
                                                   QApplication::translate("DialogSeamAllowance", "Passmark"));

            QAction *actionNonePassmark =
                passmarkSubmenu->addAction(QApplication::translate("DialogSeamAllowance", "None"));
            actionNonePassmark->setCheckable(true);
            actionNonePassmark->setChecked(!rowNode.IsPassmark());
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::NonePassmark), actionNonePassmark);

            Q_STATIC_ASSERT_X(static_cast<int>(PassmarkLineType::LAST_ONE_DO_NOT_USE) == 9,
                              "Not all types were handled.");

            auto InitPassmarkLineTypeAction = [passmarkSubmenu, rowNode](const QString &name, PassmarkLineType lineType)
            {
                QAction *action = passmarkSubmenu->addAction(name);
                action->setCheckable(true);
                action->setChecked(rowNode.IsPassmark() && lineType == rowNode.GetPassmarkLineType());
                return action;
            };

            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::OneLine),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "One line"),
                                                          PassmarkLineType::OneLine));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TwoLines),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "Two lines"),
                                                          PassmarkLineType::TwoLines));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::ThreeLines),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "Three lines"),
                                                          PassmarkLineType::ThreeLines));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TMark),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "T mark"),
                                                          PassmarkLineType::TMark));
            contextMenu.insert(
                static_cast<int>(MainPathContextMenuOption::ExternalVMark),
                InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "External V mark"),
                                           PassmarkLineType::ExternalVMark));
            contextMenu.insert(
                static_cast<int>(MainPathContextMenuOption::InternalVMark),
                InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "Internal V mark"),
                                           PassmarkLineType::InternalVMark));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::UMark),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "U mark"),
                                                          PassmarkLineType::UMark));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::BoxMark),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "Box mark"),
                                                          PassmarkLineType::BoxMark));
            contextMenu.insert(static_cast<int>(MainPathContextMenuOption::CheckMark),
                               InitPassmarkLineTypeAction(QApplication::translate("DialogSeamAllowance", "Check mark"),
                                                          PassmarkLineType::CheckMark));
        }

        QAction *actionUniqueness = menu->addAction(FromTheme(VThemeIcon::DrawStar),
                                                    QApplication::translate("DialogSeamAllowance", "Check uniqueness"));
        actionUniqueness->setCheckable(true);
        actionUniqueness->setChecked(rowNode.IsCheckUniqueness());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Uniqueness), actionUniqueness);

        QAction *actionTurnPoint = menu->addAction(FromTheme(VThemeIcon::SnapNodesRotationCenter),
                                                   QApplication::translate("DialogSeamAllowance", "Turn point"));
        actionTurnPoint->setCheckable(true);
        actionTurnPoint->setChecked(rowNode.IsTurnPoint());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TurnPoint), actionTurnPoint);
    }

    QAction *actionExcluded = menu->addAction(FromTheme(VThemeIcon::FormatTextStrikethrough),
                                              QApplication::translate("DialogSeamAllowance", "Excluded"));
    actionExcluded->setCheckable(true);
    actionExcluded->setChecked(rowNode.IsExcluded());
    contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Excluded), actionExcluded);

    QAction *actionDelete =
        menu->addAction(FromTheme(VThemeIcon::EditDelete), QApplication::translate("DialogSeamAllowance", "Delete"));
    actionDelete->setShortcut(QKeySequence::Delete);
    contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Delete), actionDelete);

    return contextMenu;
}
