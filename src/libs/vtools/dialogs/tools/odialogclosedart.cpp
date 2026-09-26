/************************************************************************
 **
 **  @file   odialogclosedart.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The dialog of the tool that closes a dart, see odialogclosedart.h.
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

#include "odialogclosedart.h"

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QSharedPointer>
#include <QStringList>
#include <QTabBar>
#include <QToolButton>
#include <QtNumeric>
#include <algorithm>

#include "../../tools/drawTools/operation/oclosedartrule.h"
#include "../../visualization/line/operation/ovistoolclosedart.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../widgets/formulainputwidget.h"
#include "ui_odialogclosedart.h"

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
ODialogCloseDart::ODialogCloseDart(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogCloseDart)
{
    ui->setupUi(this);

    // The angle belongs to the dart, so there is no angle field here — and a length would say nothing of a
    // closure. What is left to ask is how much of the opening to travel: 1, the whole of it, is the ordinary
    // answer, and anything above leaves the legs apart.
    m_formulaDivisor = new FormulaInputWidget(this);
    m_formulaDivisor->setData(&this->data);
    m_formulaDivisor->setToolId(toolId);
    m_formulaDivisor->setLabelText(tr("Closing divisor:"));
    m_formulaDivisor->setDialogTitle(tr("Edit the closing divisor"));
    // A new dart is closed all the way, and 1 is what the field offers: the angle belongs to the dart, so
    // there is nothing else for the dialog to propose.
    m_formulaDivisor->setFormula(OCloseDartRule::DefaultDivisorFormula());
    connect(m_formulaDivisor, &FormulaInputWidget::valueChanged, this, &ODialogCloseDart::DivisorChanged);

    InsertFormulaField(ui->formulaDivisorPlaceholder, m_formulaDivisor);
    ui->formulaDivisorPlaceholder = nullptr; // the placeholder is gone (deleteLater): forget the pointer

    // The field is created here, after the .ui, so Qt appends it to the end of the window's focus chain:
    // without this it would come after the OK/Cancel buttons although it sits above them.
    QWidget *const divisorEditor = m_formulaDivisor->editor();
    QWidget::setTabOrder(ui->plainTextEditToolNotes, divisorEditor);
    QWidget::setTabOrder(divisorEditor, ui->buttonBox);

    // The two points of the dart that are not the pivot live in combos of their own, never shown: validating a
    // picked object is the base class's job and it reads a combo. The three points are the ones clicked in the
    // pattern, so none of their names ever reaches the screen.
    m_comboMovingPoint = MakeComboBox(this);
    m_comboTargetPoint = MakeComboBox(this);
    m_comboMovingPoint->setVisible(false);
    m_comboTargetPoint->setVisible(false);

    MakeFilterableComboBox(ui->comboBoxNewObject);

    InitOkCancelApply(ui);

    FillComboBoxObjectTypes(ui->comboBoxObjectType);

    FillComboBoxPoints(ui->comboBoxOriginPoint);
    FillComboBoxPoints(m_comboMovingPoint);
    FillComboBoxPoints(m_comboTargetPoint);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         OperationLineStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                                 ui->comboBoxPenStyle->palette().color(QPalette::Text)),
                         TypeLineDefault);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);

    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &DialogOperationTool::ShowSourceDetails);
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &DialogOperationTool::CurrentObjectChanged);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DialogOperationTool::NameChanged);
    connect(ui->comboBoxPenStyle, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogOperationTool::PenStyleChanged);
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogOperationTool::ColorChanged);
    connect(ui->toolButtonBulkRename, &QToolButton::clicked, this, &DialogOperationTool::BulkRename);

    connect(ui->comboBoxObjectType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogOperationTool::ObjectTypeChanged);
    connect(ui->comboBoxNewObject, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogOperationTool::NewObjectChanged);
    connect(ui->toolButtonNewObject, &QPushButton::clicked, this, &DialogOperationTool::AddNewObject);
    connect(ui->toolButtonRemoveObject, &QPushButton::clicked, this, &DialogOperationTool::RemoveObject);

    ObjectTypeChanged(ui->comboBoxObjectType->currentIndex());

    vis = new OVisToolCloseDart(&this->data);

    ui->tabWidget->setCurrentIndex(0);

    // Creation/editing single page: only the closing divisor, the layer and the notes are left (every other
    // block is hidden in the .ui), so a one-tab bar would be pure noise.
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide(); // Single-page creation/editing window.
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
ODialogCloseDart::~ODialogCloseDart()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui, m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::GetOrigPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxOriginPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::SetOrigPointId(quint32 value)
{
    ChangeCurrentData(ui->comboBoxOriginPoint, value);

    if (auto *visual = qobject_cast<OVisToolCloseDart *>(vis))
    {
        visual->SetOriginPointId(value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::GetMovingPointId() const -> quint32
{
    return getCurrentObjectId(m_comboMovingPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::SetMovingPointId(quint32 value)
{
    ChangeCurrentData(m_comboMovingPoint, value);

    if (auto *visual = qobject_cast<OVisToolCloseDart *>(vis))
    {
        visual->SetMovingPointId(value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::GetTargetPointId() const -> quint32
{
    return getCurrentObjectId(m_comboTargetPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::SetTargetPointId(quint32 value)
{
    ChangeCurrentData(m_comboTargetPoint, value);

    if (auto *visual = qobject_cast<OVisToolCloseDart *>(vis))
    {
        visual->SetTargetPointId(value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::GetDivisor() const -> QString
{
    return m_formulaDivisor->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::SetDivisor(const QString &value)
{
    m_formulaDivisor->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::ShowDialog(bool click)
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

        auto *visual = qobject_cast<OVisToolCloseDart *>(vis);
        SCASSERT(visual != nullptr)
        visual->StartAction();

        scene->ToggleArcSegmentVisibility(false);
        scene->ToggleElArcSegmentVisibility(false);
        scene->ToggleSplineSegmentVisibility(false);
        scene->ToggleSplinePathSegmentVisibility(false);

        scene->ToggleArcHover(false);
        scene->ToggleElArcHover(false);
        scene->ToggleSplineHover(false);
        scene->ToggleSplinePathHover(false);

        VAbstractValApplication::VApp()->getSceneView()->AllowRubberBand(false);

        FillDefSourceNames(SourceObjects(), &data, "c"_L1);
        FillSourceList();

        m_step = Step::Pivot;
        emit ToolTip(tr("Select the pivot point"));
        return;
    }

    if (IsStage1())
    {
        return; // a click that is not the end of the selection answers no step of the dart
    }

    if (m_swallowRelease)
    {
        // The release that has just picked a point must not be read as the click that ends the next step.
        m_swallowRelease = false;
        return;
    }

    if (m_step == Step::Complete && click)
    {
        auto *visual = qobject_cast<OVisToolCloseDart *>(vis);
        SCASSERT(visual != nullptr)
        visual->SetDivisor(m_formulaDivisor->value());
        visual->RefreshGeometry();

        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::ChosenObject(quint32 id, const SceneObject &type)
{
    if (IsStage1())
    {
        if (type == SceneObject::Line)
        {
            // A line is not an operated object: it travels beside them, and its two endpoints decide whether
            // it can be transformed at all.
            ToggleLineSource(id);
        }
        else
        {
            ToggleSourceObject(id);
        }

        auto *visual = qobject_cast<OVisToolCloseDart *>(vis);
        SCASSERT(visual != nullptr)
        visual->SetObjects(SourceToObjects(SourceObjects()));
        visual->SetPreviewLines(PreviewLines());
        if (!VAbstractValApplication::VApp()->getCurrentScene()->items().contains(visual))
        {
            visual->VisualMode(NULL_ID);
        }
        else
        {
            visual->RefreshGeometry();
        }
        return;
    }

    // A dart is three points: anything else clicked now answers nothing, and the step does not move.
    if (type != SceneObject::Point)
    {
        return;
    }

    switch (m_step)
    {
        case Step::Pivot:
            TakePivot(id);
            break;
        case Step::Moving:
            TakeMovingPoint(id);
            break;
        case Step::Target:
            TakeTargetPoint(id);
            break;
        case Step::Objects:
        case Step::Complete:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::TakePivot(quint32 id)
{
    auto *visual = qobject_cast<OVisToolCloseDart *>(vis);
    SCASSERT(visual != nullptr)

    if (auto obj = std::find_if(SourceObjects().begin(), SourceObjects().end(),
                                [id](const SourceItem &sItem) { return sItem.id == id; });
        obj != SourceObjects().end())
    {
        if (SourceObjects().size() > 1)
        {
            // It is not logical for a user that the pivot cannot be selected: the same silent removal as
            // Rotation, since a rotation centre is its own image.
            SourceObjects().erase(obj);
            visual->SetObjects(SourceToObjects(SourceObjects()));
            visual->SetPreviewLines(PreviewLines());
        }
        else
        {
            Refuse(tr("This point cannot be the pivot point. Please, select another point"));
            return;
        }
    }

    if (!SetObject(id, ui->comboBoxOriginPoint, QString()))
    {
        return;
    }

    auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
    SCASSERT(window != nullptr)
    connect(visual, &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

    prepare = true;

    visual->SetOriginPointId(id);
    visual->SetMovingPointId(NULL_ID);
    visual->SetTargetPointId(NULL_ID);
    visual->RefreshGeometry();

    m_step = Step::Moving;
    m_swallowRelease = true;
    emit ToolTip(tr("Select the Moving point: the one that will close onto the other one"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::TakeMovingPoint(quint32 id)
{
    if (id == GetOrigPointId())
    {
        Refuse(tr("The Moving point must be another point than the pivot point"));
        return;
    }

    if (!SetObject(id, m_comboMovingPoint, QString()))
    {
        return;
    }

    RefreshPreview();

    m_step = Step::Target;
    m_swallowRelease = true;
    emit ToolTip(tr("Select the Target point: the one the Moving point lands on"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::TakeTargetPoint(quint32 id)
{
    if (id == GetOrigPointId() || id == GetMovingPointId())
    {
        Refuse(tr("The Target point must be a third point, different from the pivot and the Moving point"));
        return;
    }

    // The point the dart closes onto does not travel: it cannot be one of the objects that are turned.
    if (std::any_of(SourceObjects().begin(), SourceObjects().end(),
                    [id](const SourceItem &sItem) { return sItem.id == id; }))
    {
        Refuse(tr("The Target point does not move, so it cannot be one of the objects to rotate either. "
                  "Remove it from the selection, or choose another point"));
        return;
    }

    if (!SetObject(id, m_comboTargetPoint, QString()))
    {
        return;
    }

    RefreshPreview();

    // The dart is complete: the angle exists, so the release that picked this point may open the dialog.
    m_step = Step::Complete;
    m_swallowRelease = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::Refuse(const QString &message)
{
    emit ToolTip(message); // the status line keeps saying it once the box is gone

    // Over the main window, not over this dialog: the dialog is not on screen yet while the points are picked.
    // One button, and the step is still the one to answer: there is nothing to choose.
    QMessageBox::information(VAbstractValApplication::VApp()->getMainWindow(), tr("Close dart"), message);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::OnSourceObjectsSet()
{
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::RefreshPreview()
{
    if (auto *visual = qobject_cast<OVisToolCloseDart *>(vis))
    {
        // The three points live in the dialog's own combos: the preview is told about them here, so that the
        // ghost follows the mouse as soon as the point that travels is known — SetObject() fills a combo and
        // knows nothing about the visualization.
        visual->SetObjects(SourceToObjects(SourceObjects()));
        // Adding a point can add the implicit line its own tool draws, so the preview lines follow.
        visual->SetPreviewLines(PreviewLines());
        visual->SetOriginPointId(GetOrigPointId());
        visual->SetMovingPointId(GetMovingPointId());
        visual->SetTargetPointId(GetTargetPointId());
        visual->SetDivisor(m_formulaDivisor->value());
        visual->RefreshGeometry();
    }
    else
    {
        qCDebug(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::ShowVisualization()
{
    AddVisualization<OVisToolCloseDart>();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::SaveData()
{
    SourceObjects() = SaveSourceObjects();

    RefreshPreview();

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
void ODialogCloseDart::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        // The label now lives inside its widget, out of retranslateUi's reach.
        m_formulaDivisor->setLabelText(tr("Closing divisor:"));
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::PointsAreDistinct() const -> bool
{
    const quint32 pivot = GetOrigPointId();
    const quint32 moving = GetMovingPointId();
    const quint32 target = GetTargetPointId();

    return pivot != NULL_ID && moving != NULL_ID && target != NULL_ID && pivot != moving && pivot != target
           && moving != target;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogCloseDart::DivisorChanged()
{
    RefreshPreview();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogCloseDart::IsValid() const -> bool
{
    // A dart is three points that tell each other apart, and a divisor that can divide: there is nothing else
    // to be wrong about, and OK stays disabled until both hold.
    const bool pointsOk = PointsAreDistinct();
    const bool divisorOk = m_formulaDivisor->isValid()
                           && OCloseDartRule::IsDivisorUsable(m_formulaDivisor->value());

    const bool ready = pointsOk && divisorOk && IsFlagName() && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
        ui->labelStatus->setVisible(false); // nothing to report: the single-page dialog keeps the line hidden
    }
    else if (m_formulaDivisor->isValid())
    {
        // OK is disabled while the fields read fine: the line is the only place left to say why, otherwise the
        // dialog simply looks stuck. IsValid() is const, hence the two lines instead of ShowError().
        QString reason;
        if (!pointsOk)
        {
            reason = tr("The pivot, the Moving point and the Target point must be three different points");
        }
        else if (!divisorOk)
        {
            reason = tr("The closing divisor cannot be zero");
        }
        else if (!IsFlagName())
        {
            reason = tr("Invalid name");
        }
        else if (!IsFlagGroupName())
        {
            reason = tr("Invalid layer name");
        }

        if (!reason.isEmpty())
        {
            ui->labelStatus->setText(reason);
            ui->labelStatus->setVisible(true);
        }
    }

    return ready;
}
