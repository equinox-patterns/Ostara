/************************************************************************
 **
 **  @file   odialogdartcap.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  dialog of the cap of a dart
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

#include "odialogdartcap.h"
#include "ui_odialogdartcap.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPushButton>
#include <QSharedPointer>
#include <QSignalBlocker>
#include <QWidget>

#include "../../tools/drawTools/operation/oedgereader.h"
#include "../../tools/drawTools/toolpoint/toolsinglepoint/odartcaprule.h"
#include "../../visualization/line/ovistooldartcap.h"
#include "../dialogtoolbox.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vmaingraphicsview.h"

//---------------------------------------------------------------------------------------------------------------------
ODialogDartCap::ODialogDartCap(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::ODialogDartCap),
    pointName()
{
    ui->setupUi(this);

    // The four points are held in combos of their own, filled like every point combo and never shown: the
    // validation of a picked object goes through a combo, and none of these names reaches the screen.
    m_comboApex = MakeComboBox(this);
    m_comboFirstPoint = MakeComboBox(this);
    m_comboSecondPoint = MakeComboBox(this);
    m_comboExtendPoint = MakeComboBox(this);

    // They hold the four ids the base class's validation reads, and nothing of them may reach the screen: a
    // combo without a layout would otherwise be painted in the corner of the window.
    m_comboApex->setVisible(false);
    m_comboFirstPoint->setVisible(false);
    m_comboSecondPoint->setVisible(false);
    m_comboExtendPoint->setVisible(false);

    InitOkCancelApply(ui);

    m_showLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutPoint);
    connect(m_showLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { SetShowLabel(checked); });

    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    FillComboBoxPoints(m_comboApex);
    FillComboBoxPoints(m_comboFirstPoint);
    FillComboBoxPoints(m_comboSecondPoint);
    FillComboBoxPoints(m_comboExtendPoint);

    FillComboBoxTypeLine(ui->comboBoxLineType,
                         LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                        ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    connect(ui->checkBoxSecondSense, &QCheckBox::toggled, this, &ODialogDartCap::SetSecondSense);
    connect(ui->checkBoxNotch, &QCheckBox::toggled, this, &ODialogDartCap::SetNotchDrawn);

    connect(ui->checkBoxDrawCap, &QCheckBox::toggled, this,
            [this]()
            {
                UpdateCapControls();
                RefreshPreview();
                CheckState();
            });

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, pointName, &this->data,
                                flagName);
                CheckState();
            });

    UpdateCapControls();

    // The order the tab walks the page: the three checkboxes, the two fields of the pen, then the notes.
    QWidget::setTabOrder(ui->checkBoxDrawCap, ui->checkBoxNotch);
    QWidget::setTabOrder(ui->checkBoxNotch, ui->checkBoxSecondSense);
    QWidget::setTabOrder(ui->checkBoxSecondSense, ui->comboBoxLineType);
    QWidget::setTabOrder(ui->comboBoxLineType, m_colorCombo);
    QWidget::setTabOrder(m_colorCombo, ui->plainTextEditToolNotes);
    QWidget::setTabOrder(ui->plainTextEditToolNotes, ui->lineEditNamePoint);
    QWidget::setTabOrder(ui->lineEditNamePoint, ui->buttonBox);

    vis = new OVisToolDartCap(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
ODialogDartCap::~ODialogDartCap()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetApexId() const -> quint32
{
    return m_apexId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetApexId(quint32 value)
{
    m_apexId = value;
    setCurrentPointId(m_comboApex, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetFirstPointId() const -> quint32
{
    return m_firstPointId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetFirstPointId(quint32 value)
{
    m_firstPointId = value;
    setCurrentPointId(m_comboFirstPoint, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetSecondPointId() const -> quint32
{
    return m_secondPointId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetSecondPointId(quint32 value)
{
    m_secondPointId = value;
    setCurrentPointId(m_comboSecondPoint, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetExtendPointId() const -> quint32
{
    return m_extendPointId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetExtendPointId(quint32 value)
{
    m_extendPointId = value;
    setCurrentPointId(m_comboExtendPoint, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetDisplay(const OPleatDisplay &display)
{
    SetDrawCap(display.show);
    SetTypeLine(display.lineTypeLine);
    SetLineColor(display.lineColor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetDisplay() const -> OPleatDisplay
{
    OPleatDisplay display;
    display.show = GetDrawCap();
    display.lineTypeLine = GetTypeLine();
    display.lineColor = GetLineColor();

    // Nothing else is the dialog's: the fold line of a dart is the axis the cap is read on, and it is not drawn as
    // a figure of its own; the sides and the crank are the operation's to build.
    return display;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetDrawCap() const -> bool
{
    return ui->checkBoxDrawCap->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetDrawCap(bool value)
{
    ui->checkBoxDrawCap->setChecked(value);
    UpdateCapControls();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetPointName() const -> QString
{
    return pointName;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetPointName(const QString &value)
{
    pointName = value;
    ui->lineEditNamePoint->setText(pointName);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetPointLabelVisible(bool visible)
{
    m_showLabelCheckbox->setChecked(visible);
    SetShowLabel(visible);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::CheckDependencyTreeComplete()
{
    UpdateCapControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ShowDialog(bool click)
{
    if (m_swallowRelease)
    {
        // The release that has just picked a point must not be read as the click that ends the next step.
        m_swallowRelease = false;
        return;
    }

    if (m_step == Step::Complete && click)
    {
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ChosenObject(quint32 id, const SceneObject &type)
{
    // A cap is built on points: anything else clicked answers nothing, and the step does not move.
    if (type != SceneObject::Point)
    {
        return;
    }

    // Every point the user picks is a **source** of the operation: the figure is built from them, and an operation
    // copies what it names. The pleat feeds its own list the same way (ToggleSourceObject, protected in
    // DialogOperationTool); without it the cap would have no source, no figure and no place in the layer.
    ToggleSourceObject(id);

    switch (m_step)
    {
        case Step::Apex:
            TakeApex(id);
            break;
        case Step::FirstPoint:
            TakeFirstPoint(id);
            break;
        case Step::SecondPoint:
            TakeSecondPoint(id);
            break;
        case Step::ExtendPoint:
            TakeExtendPoint(id);
            break;
        case Step::Complete:
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::TakeApex(quint32 id)
{
    if (not SetObject(id, m_comboApex, tr("Select the tip of the dart (the point inside the piece)")))
    {
        return;
    }

    m_apexId = id;

    if (auto *visual = qobject_cast<OVisToolDartCap *>(vis))
    {
        visual->VisualMode(id);
    }

    m_step = Step::FirstPoint;
    m_swallowRelease = true;
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::TakeFirstPoint(quint32 id)
{
    if (id == m_apexId)
    {
        Refuse(tr("The two points of the opening are two different points, and neither of them is the point of "
                  "the dart."));
        return;
    }

    if (not SetObject(id, m_comboFirstPoint, tr("Select the first point of the opening (a point of the piece's edge)")))
    {
        return;
    }

    m_firstPointId = id;
    m_step = Step::SecondPoint;
    m_swallowRelease = true;
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::TakeSecondPoint(quint32 id)
{
    if (id == m_apexId || id == m_firstPointId)
    {
        Refuse(tr("The two points of the opening are two different points, and neither of them is the point of "
                  "the dart."));
        return;
    }

    if (not SetObject(id, m_comboSecondPoint, tr("Select the second point of the opening (the other end)")))
    {
        return;
    }

    m_secondPointId = id;

    // The two points of the opening are picked: the edge of the piece is read at the end the sense designates,
    // and the window opens on its own when that reading is enough. Nothing read there means the user has to
    // designate a point of the line to extend — the way out, exactly as the pleat asks for one.
    if (EdgeFound())
    {
        m_step = Step::Complete;
        m_swallowRelease = false;
        emit ToolTip(QString()); // the edge was read: there is nothing more to ask here
    }
    else
    {
        m_step = Step::ExtendPoint;
        m_swallowRelease = true;
        emit ToolTip(tr("Select a point of the line to extend"));
    }

    RefreshPreview();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::TakeExtendPoint(quint32 id)
{
    if (not SetObject(id, m_comboExtendPoint, QString()))
    {
        return;
    }

    m_extendPointId = id;
    flagPoint = PointsAreDistinct();

    if (not ConstructionExists())
    {
        // The cap cannot be built with this line: the dart is kept as it is and the step is asked again, so
        // that the user only has to pick another point of the line to extend.
        m_extendPointId = NULL_ID;
        m_comboExtendPoint->setCurrentIndex(-1);
        flagPoint = false;
        Refuse(tr("The cap of this dart cannot be built from this point: the line it carries does not meet the "
                  "axis of the dart. Choose another point of the line to extend."));
        RefreshPreview();
        CheckState();
        return;
    }

    // The dart cap is complete: the release that picked this point may open the window.
    m_step = Step::Complete;
    m_swallowRelease = false;
    RefreshPreview();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::Refuse(const QString &message)
{
    emit ToolTip(message); // the status line keeps saying it once the box is gone

    // Over the main window, not over this dialog: the dialog is not on screen yet while the points are picked.
    // One button, and the step is still the one to answer: there is nothing to choose.
    QMessageBox::information(VAbstractValApplication::VApp()->getMainWindow(), tr("Dart cap"), message);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::RefreshPreview()
{
    if (auto *visual = qobject_cast<OVisToolDartCap *>(vis))
    {
        // The four points live in the dialog's own combos: the preview is told about them here, because
        // SetObject() fills a combo and knows nothing about the visualization.
        visual->SetApexId(GetApexId());
        visual->SetFirstPointId(GetFirstPointId());
        visual->SetSecondPointId(GetSecondPointId());
        visual->SetExtendPointId(GetExtendPointId());
        visual->SetSecondSense(GetSecondSense());
        visual->SetEdgeDirection(EdgeDirection());
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, exactly as the pleat's dialog builds its own.
        vis = new OVisToolDartCap(&this->data);
    }

    AddVisualization<OVisToolDartCap>();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SaveData()
{
    pointName = ui->lineEditNamePoint->text();
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::PointsAreDistinct() const -> bool
{
    const quint32 apex = GetApexId();
    const quint32 first = GetFirstPointId();
    const quint32 second = GetSecondPointId();

    return apex != NULL_ID && first != NULL_ID && second != NULL_ID && apex != first && apex != second
           && first != second;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetSecondSense() const -> bool
{
    return m_secondSense;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetSecondSense(bool value)
{
    if (m_secondSense == value)
    {
        return;
    }

    m_secondSense = value;

    if (ui->checkBoxSecondSense->isChecked() != value)
    {
        const QSignalBlocker blocker(ui->checkBoxSecondSense);
        ui->checkBoxSecondSense->setChecked(value);
    }

    RefreshPreview();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetNotchDrawn() const -> bool
{
    return m_notchDrawn;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetNotchDrawn(bool value)
{
    if (m_notchDrawn == value)
    {
        return;
    }

    m_notchDrawn = value;

    if (ui->checkBoxNotch->isChecked() != value)
    {
        const QSignalBlocker blocker(ui->checkBoxNotch);
        ui->checkBoxNotch->setChecked(value);
    }

    RefreshPreview();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::EdgeDirection() const -> QPointF
{
    // The edge is read at the opening point the SENSE designates, exactly as the figure reads it: the second point is
    // the COPY of the first, and the operation that made it copied the curves of that point too — so a curve ends
    // there as well, and the reader answers on it.
    const quint32 baseId = m_secondSense ? m_secondPointId : m_firstPointId;
    QPointF direction = OEdgeReader::Direction(baseId, &data);
    if (not direction.isNull())
    {
        return direction;
    }

    // No curve ends at that point: it is a copy whose operation copied the point and not the curve. The figure then
    // reads the edge at the OTHER point of the opening, which a pleat made the image of the very same point, and
    // carries the figure by the offset between the two — the same question, asked the same way (see OToolDartCap).
    const quint32 otherId = m_secondSense ? m_firstPointId : m_secondPointId;
    return (otherId > NULL_ID) ? OEdgeReader::Direction(otherId, &data) : QPointF();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::EdgeFound() const -> bool
{
    if (not PointsAreDistinct())
    {
        return false;
    }

    // The question the figure asks, and no other: does the pattern carry an edge at the opening point the sense
    // reads? When it does not, the user is asked for the point that carries it — the same way out the pleat asks
    // for, and the figure then draws the straight tracés.
    return not EdgeDirection().isNull();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::ConstructionExists() const -> bool
{
    if (not PointsAreDistinct())
    {
        return false;
    }

    if (GetExtendPointId() == NULL_ID)
    {
        return EdgeFound(); // no point was designated: the edge of the piece is what carries the line
    }

    try
    {
        const QSharedPointer<VPointF> apex = data.GeometricObject<VPointF>(GetApexId());
        const QSharedPointer<VPointF> first = data.GeometricObject<VPointF>(GetFirstPointId());
        const QSharedPointer<VPointF> second = data.GeometricObject<VPointF>(GetSecondPointId());
        const QSharedPointer<VPointF> extend = data.GeometricObject<VPointF>(GetExtendPointId());

        QPointF capPoint;
        return not ODartCapRule::IsFailure(ODartCapRule::CapPoint(
            static_cast<QPointF>(*apex), static_cast<QPointF>(*first), static_cast<QPointF>(*second),
            static_cast<QPointF>(*extend), &capPoint));
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::UpdateCapControls()
{
    // Nothing can be edited while the pattern graph is not complete: the fields have no points to work with.
    const bool ready = m_doc->IsPatternGraphComplete();
    const bool capOn = ui->checkBoxDrawCap->isChecked() && ready;

    ui->checkBoxDrawCap->setEnabled(ready);
    ui->checkBoxNotch->setEnabled(capOn);
    ui->comboBoxLineType->setEnabled(ready);
    if (m_colorCombo != nullptr)
    {
        m_colorCombo->setEnabled(ready);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::IsValid() const -> bool
{
    // Four points that tell each other apart, and a construction that exists — asked before the window opens.
    // Nothing else can be wrong.
    return flagName && flagPoint;
}
