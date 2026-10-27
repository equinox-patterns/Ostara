/************************************************************************
 **
 **  @file   dialogrotation.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   10 4, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 12/10/2026.
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

#include "dialogrotation.h"

#include "../../tools/drawTools/operation/orotationlength.h"

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLineF>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSharedPointer>
#include <QStringList>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <qnumeric.h>

#include "../../visualization/line/operation/vistoolrotation.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vpointf.h"
#include "vtheme.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vlineedit.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../widgets/angleformulawidget.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogrotation.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
DialogRotation::DialogRotation(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogRotation)
{
    ui->setupUi(this);

    // A rotation is a circle drawn around the pivot, so it can be told either by an angle or by a length: two
    // ways of saying the same thing, and both fields stay editable. Editing one updates the other; only a real
    // change is written back, which is what keeps the two from answering each other forever. The length is the
    // chord of the reference point, the first source point (see Rotation.md).
    m_formulaAngle = new AngleFormulaWidget(this);
    m_formulaAngle->setData(&this->data);
    m_formulaAngle->setToolId(toolId);
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &AngleFormulaWidget::valueChanged, this, &DialogRotation::AngleChanged);

    m_formulaLength = new FormulaInputWidget(this);
    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    m_formulaLength->setDialogTitle(tr("Edit length"));
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this, &DialogRotation::LengthChanged);

    InsertFormulaField(ui->formulaAnglePlaceholder, m_formulaAngle);
    ui->formulaAnglePlaceholder = nullptr; // the placeholder is gone (deleteLater): forget the pointer
    InsertFormulaField(ui->formulaLengthPlaceholder, m_formulaLength);
    ui->formulaLengthPlaceholder = nullptr;

    // Both fields are created here, after the .ui, so Qt appends them to the end of the window's focus chain:
    // without this they come after the OK/Cancel buttons, although they sit above them. Notes → angle →
    // length → buttons, which is the order the dialog reads in.
    QWidget *const angleEditor = m_formulaAngle->editor();
    QWidget *const lengthEditor = m_formulaLength->editor();
    QWidget::setTabOrder(ui->plainTextEditToolNotes, angleEditor);
    QWidget::setTabOrder(angleEditor, lengthEditor);
    QWidget::setTabOrder(lengthEditor, ui->buttonBox);


    InitOkCancelApply(ui);

    UpdateReferencePoint();


    FillComboBoxPoints(ui->comboBoxOriginPoint);

    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);
    connect(ui->comboBoxOriginPoint, &QComboBox::currentTextChanged, this, &DialogRotation::PointChanged);



    vis = new VisToolRotation(&this->data);

    ui->tabWidget->setCurrentIndex(0);

    // Creation/editing single page: only the angle, the layer and the notes are left (every other
    // block is hidden in the .ui), so a one-tab bar would be pure noise.
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide(); // Single-page creation/editing window.
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogRotation::~DialogRotation()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::GetOrigPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxOriginPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::SetOrigPointId(quint32 value)
{
    ChangeCurrentData(ui->comboBoxOriginPoint, value);
    auto *operation = qobject_cast<VisToolRotation *>(vis);
    SCASSERT(operation != nullptr)
    operation->SetOriginPointId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::GetAngle() const -> QString
{
    return m_formulaAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::UserAngle() const -> QString
{
    return VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        m_formulaAngle->formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::ReferenceRadius() const -> qreal
{
    return ORotationLength::Radius(&data, GetOrigPointId(), m_referencePointId);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateLengthFromAngle()
{
    const qreal radius = ReferenceRadius();
    if (!ORotationLength::HasRadius(radius) || !m_formulaAngle->isValid())
    {
        return;
    }

    // The chord of the reference point, signed like the rotation it comes from: what an angle means as a
    // length, in the unit the user reads (see ORotationLength).
    const qreal length = ORotationLength::LengthOfAngle(radius, m_formulaAngle->value(),
                                                        *data.GetPatternUnit());

    // A length derived from the angle is always on the circle, so nothing is left to refuse.
    m_lengthIsPossible = true;

    // Remember what the dialog puts in the field: it evaluates on a timer, so this very value comes back
    // through LengthChanged a moment later and must not be read as a length the user typed.
    m_derivedLength = length;
    m_derivedLengthSet = true;

    // A value the field already says, to the precision it shows, is left alone: writing it back would only
    // disturb the field being typed in — and the two fields would keep answering each other.
    if (m_formulaLength->isValid()
        && qAbs(m_formulaLength->value() - length) <= ORotationLength::DisplayTolerance)
    {
        return;
    }

    m_formulaLength->setFormula(FormatDisplayNumber(length));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateAngleFromLength()
{
    if (!m_formulaLength->isValid())
    {
        return; // an empty or unreadable formula: the field itself says so, there is no length to convert
    }

    const qreal typedLength = m_formulaLength->value();

    // The field is showing a length this dialog derived: reading it back would answer the angle with the
    // rounding of its own display — and near a half-turn that rounding moves the angle by whole degrees.
    if (m_formulaAngle->isValid() && m_derivedLengthSet
        && qAbs(typedLength - m_derivedLength) <= ORotationLength::DisplayTolerance)
    {
        m_lengthIsPossible = true;
        return;
    }

    const qreal radius = ReferenceRadius();
    const qreal angle = ORotationLength::AngleOfLength(radius, typedLength, *data.GetPatternUnit());

    m_lengthIsPossible = !qIsNaN(angle);

    if (!m_lengthIsPossible)
    {
        // A point cannot move further from itself than the diameter of the circle it draws around the pivot.
        ShowError(tr("The length cannot exceed the circle diameter (%1%2).")
                      .arg(ORotationLength::DiameterText(radius, *data.GetPatternUnit()),
                           UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
        return;
    }

    if (m_formulaAngle->isValid()
        && qAbs(m_formulaAngle->value() - angle) <= ORotationLength::DisplayTolerance)
    {
        return; // the angle already says that: same guard as above, in the other direction
    }

    m_formulaAngle->setFormula(FormatDisplayNumber(angle));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateReferencePoint()
{
    m_referencePointId = ORotationLength::ReferencePoint(&data, SourceToObjects(SourceObjects()));

    // The length holds for one point only, and that point needs a circle around the pivot: without one there
    // is no length to speak of, so the whole field is left out rather than offered and refused.
    const bool available = m_referencePointId != NULL_ID && ORotationLength::HasRadius(ReferenceRadius());
    m_lengthAvailable = available;
    m_formulaLength->setVisible(available);

    if (!available)
    {
        // The field is gone, so whatever it had to say about the circle is gone with it: keeping the refusal
        // would lock OK on a length that is no longer asked for.
        m_lengthIsPossible = true;
        return; // nothing to derive: the angle is the only way left to tell the rotation
    }

    // The reference may have changed: the length has to be told again.
    UpdateLengthFromAngle();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::ShowError(const QString &message)
{
    ui->labelStatus->setText(message);
    ui->labelStatus->setVisible(true); // the single-page dialog hides the line while everything is fine
}

//---------------------------------------------------------------------------------------------------------------------
// Both fields are editable and each one updates the other, so a change is always the user's — except the value
// just derived, which UpdateLengthFromAngle/UpdateAngleFromLength recognise and do not write back.
void DialogRotation::AngleChanged(bool valid)
{
    if (valid)
    {
        UpdateLengthFromAngle();
    }
    else
    {
        ShowError(tr("Invalid angle formula"));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::LengthChanged(bool valid)
{
    if (valid)
    {
        UpdateAngleFromLength();
    }
    else
    {
        ShowError(tr("Invalid length formula"));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::SetAngle(const QString &value)
{
    // value comes from the pattern, in internal notation. The field shows it to the user, and the
    // visualization evaluates a user formula — hence UserAngle(), exactly like DialogEndLine::SetAngle.
    m_formulaAngle->setFormula(value);
    UpdateLengthFromAngle(); // the length tells the same rotation, as soon as the angle is evaluated

    if (auto *operation = qobject_cast<VisToolRotation *>(vis))
    {
        operation->SetAngle(UserAngle());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::ShowDialog(bool click)
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

        auto *operation = qobject_cast<VisToolRotation *>(vis);
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

        FillDefSourceNames(SourceObjects(), &data, CopySuffix::Rotation);

        emit ToolTip(tr("Select the pivot point"));
    }
    else if (not IsStage1() && prepare && click)
    {
        // The check need to ignore first release of mouse button.
        // User can select point by clicking on a label.
        if (not m_firstRelease)
        {
            m_firstRelease = true;
            return;
        }

        /*We will ignore click if pointer is in point circle*/
        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        try
        {
            const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(GetOrigPointId());
            const auto line = QLineF(static_cast<QPointF>(*point), scene->getScenePos());

            // Radius of point circle, but little bigger. Need handle with hover sizes.
            if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
            {
                return;
            }
        }
        catch (const VExceptionBadId &)
        {
            return;
        }

        auto *operation = qobject_cast<VisToolRotation *>(vis);
        SCASSERT(operation != nullptr)

        SetAngle(operation->Angle()); // Show in dialog angle that a user choose
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::ChosenObject(quint32 id, const SceneObject &type)
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
        auto *operation = qobject_cast<VisToolRotation *>(vis);
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
    else if (not prepare && type == SceneObject::Point) // After first choose we ignore all objects
    {
        auto *operation = qobject_cast<VisToolRotation *>(vis);
        SCASSERT(operation != nullptr)

        if (auto obj = std::find_if(SourceObjects().begin(),
                                    SourceObjects().end(),
                                    [id](const SourceItem &sItem) { return sItem.id == id; });
            obj != SourceObjects().end())
        {
            if (SourceObjects().size() > 1)
            {
                // It's not really logical for a user that a center of rotation no need to select.
                // To fix this issue we just silently remove it from the list.
                SourceObjects().erase(obj);
                operation->SetObjects(SourceToObjects(SourceObjects()));
                operation->SetPreviewLines(PreviewLines());
            }
            else
            {
                emit ToolTip(tr("This point cannot be the pivot point. Please, select another point"));
                return;
            }
        }

        if (SetObject(id, ui->comboBoxOriginPoint, QString()))
        {
            auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(operation, &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

            operation->SetOriginPointId(id);
            operation->RefreshGeometry();

            prepare = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::OnSourceObjectsSet()
{
    // The length refers to the first source point: the sources decide whether it is offered at all.
    UpdateReferencePoint();

    if (auto *operation = qobject_cast<VisToolRotation *>(vis))
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        // Adding a point can add the implicit line its own tool draws, so the preview lines follow.
        operation->SetPreviewLines(PreviewLines());
        operation->RefreshGeometry();
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, and the values the dialog shows follow.
        vis = new VisToolRotation(&this->data);
    }

    AddVisualization<VisToolRotation>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::SaveData()
{
    const QString angleFormula = UserAngle();

    if (!vis.isNull())
    {
        auto *operation = qobject_cast<VisToolRotation *>(vis);
        SCASSERT(operation != nullptr)

        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->SetOriginPointId(GetOrigPointId());
        operation->SetAngle(angleFormula);
        operation->RefreshGeometry();
    }

}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        // The two labels now live inside their widgets, out of retranslateUi's reach.
        m_formulaAngle->setLabelText(tr("Angle:"));
        m_formulaLength->setLabelText(tr("Length:"));
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::PointChanged()
{
    QColor color;
    quint32 const id = getCurrentObjectId(ui->comboBoxOriginPoint);

    if (auto obj = std::find_if(SourceObjects().begin(),
                                SourceObjects().end(),
                                [id](const SourceItem &sItem) { return sItem.id == id; });
        obj != SourceObjects().end())
    {
        SetFlagError(false);
        color = errorColor;
        ShowError(tr("Invalid rotation point"));
    }
    else
    {
        SetFlagError(true);
        color = OkColor(this);
    }
    ChangeColor(ui->labelOriginPoint, color);

    // The radius is measured from the pivot: moving it changes the length the same angle means.
    UpdateReferencePoint();

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogRotation::IsValid() const -> bool
{
    // Both fields say the same rotation: the angle one always has to be readable, since it is what gets stored,
    // and the length can only be wrong about the circle when the rotation offers one at all.
    const bool lengthOk = !m_lengthAvailable || (m_formulaLength->isValid() && m_lengthIsPossible);
    const bool rotationIsValid = m_formulaAngle->isValid() && lengthOk;

    bool const ready = rotationIsValid && IsFlagError() && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
        ui->labelStatus->setVisible(false); // nothing to report: the single-page dialog keeps the line hidden
    }
    else if (rotationIsValid)
    {
        // OK is disabled while every field reads fine: the line is the only place left to say why, otherwise
        // the dialog simply looks stuck. IsValid() is const, hence the two lines instead of ShowError().
        QString reason;
        if (!IsFlagError())
        {
            reason = tr("Invalid rotation point");
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
