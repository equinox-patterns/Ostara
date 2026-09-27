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
#include <QWidget>

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
  : DialogTool(data, doc, toolId, parent),
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

    // The two names of the tracés, each with the pastille of the colour its tracé is drawn in — the same pair
    // of colours the preview uses.
    m_line1Row = new QWidget();
    auto *rowLine1 = new QHBoxLayout(m_line1Row);
    rowLine1->setContentsMargins(0, 0, 0, 0);
    rowLine1->setSpacing(4);
    auto *dotLine1 = new QLabel();
    dotLine1->setObjectName(QStringLiteral("labelRoleNameDot"));
    dotLine1->setFixedSize(12, 12);
    m_labelLine1 = new QLabel(tr("Line:"));
    m_labelLine1->setMinimumWidth(80);
    m_lineEditLine1 = new QLineEdit();
    m_lineEditLine1->setObjectName(QStringLiteral("lineEditRoleName"));
    rowLine1->addWidget(dotLine1);
    rowLine1->addWidget(m_labelLine1);
    rowLine1->addWidget(m_lineEditLine1, 1);
    ui->verticalLayout->insertWidget(1, m_line1Row);
    connect(m_lineEditLine1, &QLineEdit::textEdited, this, &ODialogDartCap::ValidateLine1RoleName);

    m_line2Row = new QWidget();
    auto *rowLine2 = new QHBoxLayout(m_line2Row);
    rowLine2->setContentsMargins(0, 0, 0, 0);
    rowLine2->setSpacing(4);
    auto *dotLine2 = new QLabel();
    dotLine2->setObjectName(QStringLiteral("labelSecondRoleNameDot"));
    dotLine2->setFixedSize(12, 12);
    m_labelLine2 = new QLabel(tr("Line:"));
    m_labelLine2->setMinimumWidth(80);
    m_lineEditLine2 = new QLineEdit();
    m_lineEditLine2->setObjectName(QStringLiteral("lineEditSecondRoleName"));
    rowLine2->addWidget(dotLine2);
    rowLine2->addWidget(m_labelLine2);
    rowLine2->addWidget(m_lineEditLine2, 1);
    ui->verticalLayout->insertWidget(2, m_line2Row);
    connect(m_lineEditLine2, &QLineEdit::textEdited, this, &ODialogDartCap::ValidateLine2RoleName);

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

    QWidget::setTabOrder(ui->checkBoxDrawCap, m_lineEditLine1);
    QWidget::setTabOrder(m_lineEditLine1, m_lineEditLine2);
    QWidget::setTabOrder(m_lineEditLine2, ui->comboBoxLineType);
    QWidget::setTabOrder(ui->comboBoxLineType, m_colorCombo);
    QWidget::setTabOrder(m_colorCombo, ui->plainTextEditToolNotes);
    QWidget::setTabOrder(ui->plainTextEditToolNotes, ui->lineEditNamePoint);
    QWidget::setTabOrder(ui->lineEditNamePoint, ui->buttonBox);

    InitIcons();

    vis = new OVisToolDartCap(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::InitIcons()
{
    // The same two colours the canvas draws the two tracés with, and the dock shows on their names.
    if (auto *dot = findChild<QLabel *>(QStringLiteral("labelRoleNameDot")))
    {
        dot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor2), 10));
    }
    if (auto *dot = findChild<QLabel *>(QStringLiteral("labelSecondRoleNameDot")))
    {
        dot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor4), 10));
    }
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
auto ODialogDartCap::GetLine1RoleName() const -> QString
{
    return m_lineEditLine1->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetLine1RoleName(const QString &suffix)
{
    m_lineEditLine1->setText(suffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::GetLine2RoleName() const -> QString
{
    return m_lineEditLine2->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::SetLine2RoleName(const QString &suffix)
{
    m_lineEditLine2->setText(suffix);
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
    if (not SetObject(id, m_comboApex, tr("Select the first point of the dart")))
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

    if (not SetObject(id, m_comboFirstPoint, tr("Select the second point of the dart")))
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

    if (not SetObject(id, m_comboSecondPoint, tr("Select a point of the line to extend")))
    {
        return;
    }

    m_secondPointId = id;
    m_step = Step::ExtendPoint;
    m_swallowRelease = true;
    RefreshPreview();
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
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ShowVisualization()
{
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

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons(); // the two pastilles follow the theme
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
auto ODialogDartCap::ConstructionExists() const -> bool
{
    if (not PointsAreDistinct() || GetExtendPointId() == NULL_ID)
    {
        return false;
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
    ui->comboBoxLineType->setEnabled(ready);
    if (m_colorCombo != nullptr)
    {
        m_colorCombo->setEnabled(ready);
    }

    // The two names of the tracés only mean something when the cap is drawn.
    m_line1Row->setEnabled(capOn);
    m_line2Row->setEnabled(capOn);
    m_labelLine1->setEnabled(capOn);
    m_labelLine2->setEnabled(capOn);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ValidateLine1RoleName()
{
    flagLine1 = ValidateLineRoleName(m_lineEditLine1, m_labelLine1);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogDartCap::ValidateLine2RoleName()
{
    flagLine2 = ValidateLineRoleName(m_lineEditLine2, m_labelLine2);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::ValidateLineRoleName(QLineEdit *edit, QLabel *label) -> bool
{
    if (edit == nullptr || label == nullptr)
    {
        return true;
    }

    const QString suffix = edit->text().trimmed();
    if (suffix.isEmpty())
    {
        edit->setStyleSheet({});
        ChangeColor(label, OkColor(this));
        return true;
    }

    if (QRegularExpression const rx(NameRegExp()); not rx.match(suffix).hasMatch())
    {
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(label, errorColor);
        return false;
    }

    // Line_<suffix> names a pair of points: it has to stay unique among the lines of the pattern.
    const QString aliasKey = line_ + suffix;
    const bool taken = data.DataLengthLines().contains(aliasKey)
                       && data.DataLengthLines().value(aliasKey)->GetReferences().size() >= 2;
    if (taken)
    {
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(label, errorColor);
        return false;
    }

    edit->setStyleSheet({});
    ChangeColor(label, OkColor(this));
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogDartCap::IsValid() const -> bool
{
    // Four points that tell each other apart, a construction that exists — asked before the window opens — and
    // two names that can name a line. Nothing else can be wrong.
    return flagName && flagPoint && flagLine1 && flagLine2;
}
