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

#include "../../tools/drawTools/operation/ooperationlines.h"

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
#include <QRadioButton>
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

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// A derived value is meant to be read, and three decimals is already finer than anything a pattern can tell
// apart: rounding there is what keeps "5.00001" from showing up where the user asked for 5 cm. Trailing zeros
// go with it, so that a whole number is written "5" and not "5.000".
auto FormatDerivedValue(qreal value) -> QString
{
    QString text = QString::number(value, 'f', 3);

    if (text.contains(QLatin1Char('.')))
    {
        while (text.endsWith(QLatin1Char('0')))
        {
            text.chop(1);
        }

        if (text.endsWith(QLatin1Char('.')))
        {
            text.chop(1);
        }
    }

    // A value rounded down to zero has no sign left to show, and "-0" reads like a mistake.
    return text == QLatin1String("-0") ? QStringLiteral("0") : text;
}

//---------------------------------------------------------------------------------------------------------------------
// Replace a formula placeholder by a [radio][field] line: the radio tells which of the two fields sets the
// rotation, so it belongs on the line of the field it commands.
void InsertFormulaField(QWidget *placeholder, QRadioButton *radio, QWidget *field)
{
    if (placeholder == nullptr)
    {
        return;
    }

    auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
    if (parentLayout == nullptr)
    {
        return;
    }

    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(radio);
    row->addWidget(field);
    parentLayout->insertLayout(parentLayout->indexOf(placeholder), row);
    placeholder->deleteLater();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogRotation::DialogRotation(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogRotation)
{
    ui->setupUi(this);

    // A rotation is a circle drawn around the pivot, so it can be told either by an angle or by a length: two
    // ways of saying the same thing. The length is the chord of the reference point, the first source point.
    // Only the field of the checked radio is editable; the other one stays greyed but keeps showing the value
    // it derives (see Modifications/Ostara/Tools/Draw/Operations/Rotation.md).
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

    m_radioAngle = new QRadioButton(this);
    m_radioLength = new QRadioButton(this);
    connect(m_radioAngle, &QRadioButton::toggled, this,
            [this](bool checked)
            {
                // One field drives, the other one follows: the radio is what makes the derivation unambiguous.
                m_formulaAngle->setEnabled(checked);
                m_formulaLength->setEnabled(!checked);
                CheckState();
            });
    InsertFormulaField(ui->formulaAnglePlaceholder, m_radioAngle, m_formulaAngle);
    InsertFormulaField(ui->formulaLengthPlaceholder, m_radioLength, m_formulaLength);

    MakeFilterableComboBox(ui->comboBoxNewObject);

    InitOkCancelApply(ui);

    // Checking a radio emits toggled, which runs CheckState() — and CheckState() needs the OK button to exist.
    // So the angle is only made the active mode once the buttons are in place, as is the reference lookup.
    m_radioAngle->setChecked(true); // opening on the angle, as before
    UpdateReferencePoint();

    FillComboBoxObjectTypes(ui->comboBoxObjectType);

    FillComboBoxPoints(ui->comboBoxOriginPoint);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         OperationLineStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                                 ui->comboBoxPenStyle->palette().color(QPalette::Text)),
                         TypeLineDefault);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);

    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);
    connect(ui->comboBoxOriginPoint, &QComboBox::currentTextChanged, this, &DialogRotation::PointChanged);

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &DialogOperationTool::ShowSourceDetails);
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &DialogOperationTool::CurrentObjectChanged);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DialogOperationTool::NameChanged);
    connect(ui->comboBoxPenStyle, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogOperationTool::PenStyleChanged);
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialogOperationTool::ColorChanged);
    connect(ui->toolButtonBulkRename, &QToolButton::clicked, this, &DialogOperationTool::BulkRename);

    connect(ui->comboBoxObjectType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DialogOperationTool::ObjectTypeChanged);
    connect(ui->comboBoxNewObject,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DialogOperationTool::NewObjectChanged);
    connect(ui->toolButtonNewObject, &QPushButton::clicked, this, &DialogOperationTool::AddNewObject);
    connect(ui->toolButtonRemoveObject, &QPushButton::clicked, this, &DialogOperationTool::RemoveObject);

    ObjectTypeChanged(ui->comboBoxObjectType->currentIndex());

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
    return {.sourceList = ui->listWidget,
            .name = ui->lineEditName,
            .labelName = ui->labelName,
            .labelStatus = ui->labelStatus,
            .labelGroupName = ui->labelGroupName,
            .penStyle = ui->comboBoxPenStyle,
            .color = m_colorCombo,
            .objectType = ui->comboBoxObjectType,
            .newObject = ui->comboBoxNewObject,
            .addObject = ui->toolButtonNewObject,
            .removeObject = ui->toolButtonRemoveObject,
            .notes = ui->plainTextEditToolNotes,
            .groupTags = ui->lineEditGroupTags,
            .visibilityGroupLine = ui->lineEditVisibilityGroup,
            .visibilityGroup = ui->groupBoxVisibilityGroup};
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
    try
    {
        const QSharedPointer<VPointF> pivot = data.GeometricObject<VPointF>(GetOrigPointId());
        const QSharedPointer<VPointF> reference = data.GeometricObject<VPointF>(m_referencePointId);
        return QLineF(static_cast<QPointF>(*pivot), static_cast<QPointF>(*reference)).length();
    }
    catch (const VExceptionBadId &)
    {
        return 0.; // no pivot or no reference point: no radius, hence nothing to say about a length
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateLengthFromAngle()
{
    const qreal radius = ReferenceRadius();
    if (qFuzzyIsNull(radius) || !m_formulaAngle->isValid())
    {
        return;
    }

    // The chord of the reference point, signed like the rotation it comes from: a chord knows only the short
    // way round, so the sign is what says the direction (see RotationChord).
    const qreal chord = RotationChord(radius, m_formulaAngle->value());
    m_formulaLength->setFormula(FormatDerivedValue(FromPixel(chord, *data.GetPatternUnit())));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateAngleFromLength()
{
    if (!m_formulaLength->isValid())
    {
        return; // an empty or unreadable formula: the field itself says so, there is no length to convert
    }

    const qreal radius = ReferenceRadius();
    const qreal chord = ToPixel(m_formulaLength->value(), *data.GetPatternUnit());
    const qreal angle = RotationAngleForChord(radius, chord);

    m_lengthIsPossible = !qIsNaN(angle);

    if (!m_lengthIsPossible)
    {
        // A point cannot move further from itself than the diameter of the circle it draws around the pivot.
        ShowError(tr("The length cannot exceed the circle diameter (%1%2).")
                      .arg(FormatDerivedValue(FromPixel(2. * radius, *data.GetPatternUnit())),
                           UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
        return;
    }

    m_formulaAngle->setFormula(FormatDerivedValue(angle));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::UpdateReferencePoint()
{
    m_referencePointId = data.FirstPointId(SourceToObjects(SourceObjects()));

    // The length holds for one point only, and that point needs a circle around the pivot: without one there
    // is no length to speak of, so the whole field is left out rather than offered and refused.
    const bool available = m_referencePointId != NULL_ID && !qFuzzyIsNull(ReferenceRadius());
    m_radioLength->setVisible(available);
    m_formulaLength->setVisible(available);

    if (!available)
    {
        if (m_radioLength->isChecked())
        {
            m_radioAngle->setChecked(true);
        }
        return;
    }

    try
    {
        m_formulaLength->setLabelText(tr("Length %1 → %1':").arg(data.GetGObject(m_referencePointId)->name()));
    }
    catch (const VExceptionBadId &)
    {
        m_formulaLength->setLabelText(tr("Length:"));
    }

    // The reference may have changed: the length has to be told again.
    if (m_radioAngle->isChecked())
    {
        UpdateLengthFromAngle();
    }
    else
    {
        UpdateAngleFromLength();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::ShowError(const QString &message)
{
    ui->labelStatus->setText(message);
    ui->labelStatus->setVisible(true); // the single-page dialog hides the line while everything is fine
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::AngleChanged(bool valid)
{
    // Only the checked field is editable, so a change of the other one is necessarily the value derived just
    // before: nothing to compute back, and no way to loop.
    if (!m_radioAngle->isChecked())
    {
        return;
    }

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
    if (!m_radioLength->isChecked())
    {
        return;
    }

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

        FillDefSourceNames(SourceObjects(), &data, "r"_L1);
        FillSourceList();

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
        operation->SetPreviewLines(PreviewLines());
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
    AddVisualization<VisToolRotation>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogRotation::SaveData()
{
    const QString angleFormula = UserAngle();
    SourceObjects() = SaveSourceObjects();

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
void DialogRotation::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        // The angle label now lives inside the widget, out of retranslateUi's reach.
        m_formulaAngle->setLabelText(tr("Angle:"));
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
    // Only the field of the checked radio makes the rotation: the other one is derived, and never blocks OK.
    const bool rotationIsValid = m_radioLength->isChecked() ? (m_formulaLength->isValid() && m_lengthIsPossible)
                                                            : m_formulaAngle->isValid();

    bool const ready = rotationIsValid && IsFlagName() && IsFlagError() && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
        ui->labelStatus->setVisible(false); // nothing to report: the single-page dialog keeps the line hidden
    }

    return ready;
}
