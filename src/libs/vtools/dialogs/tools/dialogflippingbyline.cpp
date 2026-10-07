/************************************************************************
 **
 **  @file   dialogflippingbyline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 9, 2016
 **
 **  @brief
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

#include "dialogflippingbyline.h"

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSharedPointer>
#include <QStringList>
#include <QToolButton>

#include "../../visualization/line/operation/vistoolflippingbyline.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../qmuparser/qmudef.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vlineedit.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "ui_dialogflippingbyline.h"

#include <QTabBar>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
DialogFlippingByLine::DialogFlippingByLine(const VContainer *data,
                                           VAbstractPattern *doc,
                                           quint32 toolId,
                                           QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogFlippingByLine)
{
    ui->setupUi(this);

    InitOkCancelApply(ui);



    FillComboBoxPoints(ui->comboBoxFirstLinePoint);
    FillComboBoxPoints(ui->comboBoxSecondLinePoint);

    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this,
            &DialogOperationTool::GroupNameChanged);
    connect(ui->comboBoxFirstLinePoint, &QComboBox::currentTextChanged, this, &DialogFlippingByLine::PointChanged);
    connect(ui->comboBoxSecondLinePoint, &QComboBox::currentTextChanged, this, &DialogFlippingByLine::PointChanged);



    vis = new VisToolFlippingByLine(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide(); // Single-page creation/editing window.
    }
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogFlippingByLine::~DialogFlippingByLine()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogFlippingByLine::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogFlippingByLine::GetFirstLinePointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxFirstLinePoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::SetFirstLinePointId(quint32 value)
{
    ChangeCurrentData(ui->comboBoxFirstLinePoint, value);
    if (auto *operation = qobject_cast<VisToolFlippingByLine *>(vis))
    {
        operation->SetFirstLinePointId(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogFlippingByLine::GetSecondLinePointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxSecondLinePoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::SetSecondLinePointId(quint32 value)
{
    ChangeCurrentData(ui->comboBoxSecondLinePoint, value);
    if (auto *operation = qobject_cast<VisToolFlippingByLine *>(vis))
    {
        operation->SetSecondLinePointId(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::ShowDialog(bool click)
{
    if (IsStage1() && not click)
    {
        if (SourceObjects().isEmpty())
        {
            return;
        }

        SetStage1(false);

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        scene->clearSelection();

        auto *operation = qobject_cast<VisToolFlippingByLine *>(vis);
        SCASSERT(operation != nullptr)
        operation->StartAction();

        scene->ToggleArcSegmentVisibility(false);
        scene->ToggleElArcSegmentVisibility(false);
        scene->ToggleSplineSegmentVisibility(false);
        scene->ToggleSplinePathSegmentVisibility(false);

        scene->ToggleArcHover(false);
        scene->ToggleElArcHover(false);
        scene->ToggleSplineHover(false);
        scene->ToggleSplinePathHover(false);

        VAbstractValApplication::VApp()->getSceneView()->AllowRubberBand(false);

        FillDefSourceNames(SourceObjects(), &data, "fl"_L1);

        emit ToolTip(tr("Select first line point"));
    }
    else if (not IsStage1() && prepare && click)
    {
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::ChosenObject(quint32 id, const SceneObject &type)
{
    if (IsStage1())
    {
        if (type == SceneObject::Line)
        {
            // A line is not an operated object: it travels beside them, and its two endpoints decide
            // whether it can be transformed at all.
            ToggleLineSource(id);
        }
        else
        {
            ToggleSourceObject(id);
        }

        // Both cases must refresh the preview: a line used to return right here, so neither its highlight
        // nor its ghost could ever appear.
        auto *operation = qobject_cast<VisToolFlippingByLine *>(vis);
        SCASSERT(operation != nullptr)
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        if (!VAbstractValApplication::VApp()->getCurrentScene()->items().contains(operation))
        {
            operation->VisualMode(NULL_ID);
        }
        else
        {
            operation->RefreshGeometry();
        }
    }
    else if (!prepare && type == SceneObject::Point) // After first choose we ignore all objects
    {
        ChooseLinePoint(id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::ChooseLinePoint(quint32 id)
{
    if (const bool isSourceObject = std::find_if(SourceObjects().begin(),
                                                 SourceObjects().end(),
                                                 [id](const SourceItem &sItem) -> bool { return sItem.id == id; })
                                    != SourceObjects().end();
        number == 0)
    {
        if (isSourceObject)
        {
            emit ToolTip(tr("Select first line point that is not part of the list of objects"));
            return;
        }

        if (SetObject(id, ui->comboBoxFirstLinePoint, tr("Select second line point")))
        {
            number++;
            auto *operation = qobject_cast<VisToolFlippingByLine *>(vis);
            SCASSERT(operation != nullptr)
            operation->SetFirstLinePointId(id);
            operation->RefreshGeometry();
        }
    }
    else if (number == 1)
    {
        if (isSourceObject)
        {
            emit ToolTip(tr("Select second line point that is not part of the list of objects"));
            return;
        }

        if (getCurrentObjectId(ui->comboBoxFirstLinePoint) != id &&
            SetObject(id, ui->comboBoxSecondLinePoint, QString()) && IsFlagError())
        {
            number = 0;
            prepare = true;

            auto *operation = qobject_cast<VisToolFlippingByLine *>(vis);
            SCASSERT(operation != nullptr)
            operation->SetSecondLinePointId(id);
            operation->RefreshGeometry();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::OnSourceObjectsSet()
{
    if (auto *operation = qobject_cast<VisToolFlippingByLine *>(vis))
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->RefreshGeometry();
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, and the values the dialog shows follow.
        vis = new VisToolFlippingByLine(&this->data);
    }

    AddVisualization<VisToolFlippingByLine>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::SaveData()
{

    if (!vis.isNull())
    {
        auto *operation = qobject_cast<VisToolFlippingByLine *>(vis);
        SCASSERT(operation != nullptr)

        operation->SetObjects(SourceToObjects(SourceObjects()));

        operation->SetPreviewLines(PreviewLines());
        operation->SetFirstLinePointId(GetFirstLinePointId());
        operation->SetSecondLinePointId(GetSecondLinePointId());
        operation->RefreshGeometry();
    }

    QStringList groupTags = ui->lineEditGroupTags->text().split(',');
    for (auto &tag : groupTags)
    {
        tag = tag.trimmed();
        if (not GroupTags().contains(tag))
        {
            GroupTags().append(tag);
        }
    }

    ui->lineEditGroupTags->SetCompletion(GroupTags());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogFlippingByLine::PointChanged()
{
    QColor color = OkColor(this);
    SetFlagError(true);
    ChangeColor(ui->labelFirstLinePoint, color);
    ChangeColor(ui->labelSecondLinePoint, color);

    quint32 const id1 = getCurrentObjectId(ui->comboBoxFirstLinePoint);
    auto obj1 = std::find_if(SourceObjects().begin(),
                             SourceObjects().end(),
                             [id1](const SourceItem &sItem) -> bool { return sItem.id == id1; });

    quint32 const id2 = getCurrentObjectId(ui->comboBoxSecondLinePoint);
    auto obj2 = std::find_if(SourceObjects().begin(),
                             SourceObjects().end(),
                             [id2](const SourceItem &sItem) -> bool { return sItem.id == id2; });

    if (getCurrentObjectId(ui->comboBoxFirstLinePoint) == getCurrentObjectId(ui->comboBoxSecondLinePoint))
    {
        SetFlagError(false);
        color = errorColor;
        ChangeColor(ui->labelFirstLinePoint, color);
        ChangeColor(ui->labelSecondLinePoint, color);
        ui->labelStatus->setText(tr("Invalid line points"));
    }
    else if (obj1 != SourceObjects().end())
    {
        SetFlagError(false);
        color = errorColor;
        ChangeColor(ui->labelFirstLinePoint, color);
        ui->labelStatus->setText(tr("Invalid first line point"));
    }
    else if (obj2 != SourceObjects().end())
    {
        SetFlagError(false);
        color = errorColor;
        ChangeColor(ui->labelSecondLinePoint, color);
        ui->labelStatus->setText(tr("Invalid second line point"));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogFlippingByLine::IsValid() const -> bool
{
    bool const ready = IsFlagError() && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
    }

    return ready;
}
