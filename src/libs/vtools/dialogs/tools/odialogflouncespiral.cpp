/************************************************************************
 **
 **  @file   odialogflouncespiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The dialog of the flounce spiral.
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

#include "odialogflouncespiral.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>

#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../widgets/angleformulawidget.h"
#include "../widgets/formulainputwidget.h"
#include "dialogtoolbox.h"
#include "ui_odialogflouncespiral.h"

//---------------------------------------------------------------------------------------------------------------------
ODialogFlounceSpiral::ODialogFlounceSpiral(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                           QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::ODialogFlounceSpiral),
    m_formulaWidth(new FormulaInputWidget(this)),
    m_formulaLength(new FormulaInputWidget(this)),
    m_formulaAngle(new AngleFormulaWidget(this))
{
    ui->setupUi(this);
    qDebug() << "FLOUNCE-DEBUG: dialog construite";

    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaWidth->setData(&this->data);
    m_formulaWidth->setToolId(toolId);
    m_formulaWidth->setLabelText(tr("Width:"));
    m_formulaWidth->setPostfix(lengthPostfix);
    m_formulaWidth->setDialogTitle(tr("Edit width"));
    m_formulaWidth->setCheckZero(true);
    m_formulaWidth->setCheckLessThanZero(true);
    connect(m_formulaWidth, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(lengthPostfix);
    m_formulaLength->setDialogTitle(tr("Edit length"));
    m_formulaLength->setCheckZero(true);
    m_formulaLength->setCheckLessThanZero(true);
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

    m_formulaAngle->setData(&this->data);
    m_formulaAngle->setToolId(toolId);
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &AngleFormulaWidget::valueChanged, this, [this]() { CheckState(); });
    // The common case puts A to the left of the clicked point; anything else is a deliberate choice.
    m_formulaAngle->setFormula(QStringLiteral("180"));

    const auto replacePlaceholder = [](QWidget *placeholder, QWidget *widget) -> void
    {
        if (placeholder == nullptr)
        {
            return;
        }

        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, widget);
            placeholder->deleteLater();
        }
    };
    replacePlaceholder(ui->formulaWidthPlaceholder, m_formulaWidth);
    replacePlaceholder(ui->formulaLengthPlaceholder, m_formulaLength);
    replacePlaceholder(ui->formulaAnglePlaceholder, m_formulaAngle);

    InitOkCancelApply(ui);

    // Without this the list is empty, SetObject never finds the clicked point, and the dialog never opens: the
    // tool looks dead and says nothing. Every tool that starts from an existing point fills its list.
    MakeFilterableComboBox(ui->comboBoxStartPoint);
    FillComboBoxPoints(ui->comboBoxStartPoint);

    // ONE style for the whole figure: both edges and both joining segments are drawn with this pen and this
    // colour. There is no "outer spiral" to style apart, and no segment either: the flounce is a whole.
    m_color = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);

    const QColor base = ui->comboBoxPenStyle->palette().color(QPalette::Base);
    const QColor text = ui->comboBoxPenStyle->palette().color(QPalette::Text);
    FillComboBoxTypeLine(ui->comboBoxPenStyle, CurvePenStylesPics(base, text));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &ODialogFlounceSpiral::ValidateAlias);
    connect(ui->lineEditPoint1Name, &QLineEdit::textEdited, this, &ODialogFlounceSpiral::ValidatePointNames);
    connect(ui->lineEditPoint2Name, &QLineEdit::textEdited, this, &ODialogFlounceSpiral::ValidatePointNames);
    connect(ui->lineEditPoint3Name, &QLineEdit::textEdited, this, &ODialogFlounceSpiral::ValidatePointNames);

    m_showLabel1 = AddShowPointNameCheckbox(ui->formLayoutPoint1);
    connect(m_showLabel1, &QCheckBox::toggled, this, [this](bool checked) { Q_UNUSED(checked) CheckState(); });
    m_showLabel2 = AddShowPointNameCheckbox(ui->formLayoutPoint2);
    connect(m_showLabel2, &QCheckBox::toggled, this, [this](bool checked) { Q_UNUSED(checked) CheckState(); });
    m_showLabel3 = AddShowPointNameCheckbox(ui->formLayoutPoint3);
    connect(m_showLabel3, &QCheckBox::toggled, this, [this](bool checked) { Q_UNUSED(checked) CheckState(); });

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);

    qDebug() << "FLOUNCE-DEBUG: constructeur termine, points proposes ="
             << ui->comboBoxStartPoint->count();

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogFlounceSpiral::~ODialogFlounceSpiral()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetStartPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxStartPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetStartPointId(quint32 id)
{
    ChangeCurrentData(ui->comboBoxStartPoint, id);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetFormulaWidth() const -> QString
{
    return m_formulaWidth->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetFormulaWidth(const QString &value)
{
    m_formulaWidth->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetFormulaLength() const -> QString
{
    return m_formulaLength->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetFormulaLength(const QString &value)
{
    m_formulaLength->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetFormulaAngle() const -> QString
{
    return m_formulaAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetFormulaAngle(const QString &value)
{
    m_formulaAngle->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetDisplay() const -> OFlounceSpiralDisplay
{
    OFlounceSpiralDisplay display;
    display.width = GetFormulaWidth();
    display.length = GetFormulaLength();
    display.angle = GetFormulaAngle();
    // One style for the whole figure, written in the one pair the display carries.
    display.typeLine = GetPenStyle();
    display.lineColor = GetColor();
    // One name for one curve. The inner alias is not offered here: a figure carries the inner name of an older
    // pattern untouched, and the tool keeps it when the dialog is saved.
    display.outerAlias = GetOuterAlias();

    auto AppendPoint = [&display](OFlounceSpiralPoint::Role role, const QString &name, bool showLabel)
    {
        OFlounceSpiralPoint point;
        point.role = role;
        point.name = name;
        point.labelVisible = showLabel;
        display.points.append(point);
    };

    // The roles in the order the figure places them, each with the switch of its own box.
    AppendPoint(OFlounceSpiralPoint::Role::OuterStart, GetPointName1(), GetPointShowLabel1());
    AppendPoint(OFlounceSpiralPoint::Role::InnerEnd, GetPointName2(), GetPointShowLabel2());
    AppendPoint(OFlounceSpiralPoint::Role::OuterEnd, GetPointName3(), GetPointShowLabel3());

    return display;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetDisplay(const OFlounceSpiralDisplay &display)
{
    SetFormulaWidth(display.width);
    SetFormulaLength(display.length);
    SetFormulaAngle(display.angle);

    ChangeCurrentData(ui->comboBoxPenStyle, display.typeLine);
    SetColorComboCurrentName(m_color, display.lineColor);

    for (const OFlounceSpiralPoint &point : display.points)
    {
        switch (point.role)
        {
            case OFlounceSpiralPoint::Role::OuterStart:
                SetPointName1(point.name);
                break;
            case OFlounceSpiralPoint::Role::InnerEnd:
                SetPointName2(point.name);
                break;
            case OFlounceSpiralPoint::Role::OuterEnd:
                SetPointName3(point.name);
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetOuterAlias() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetOuterAlias(const QString &alias)
{
    m_originAlias = alias;
    ui->lineEditAlias->setText(alias);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_color);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointName1() const -> QString
{
    return ui->lineEditPoint1Name->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointName1(const QString &name)
{
    m_originPointName1 = name;
    ui->lineEditPoint1Name->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointName2() const -> QString
{
    return ui->lineEditPoint2Name->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointName2(const QString &name)
{
    m_originPointName2 = name;
    ui->lineEditPoint2Name->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointName3() const -> QString
{
    return ui->lineEditPoint3Name->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointName3(const QString &name)
{
    m_originPointName3 = name;
    ui->lineEditPoint3Name->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointShowLabel1() const -> bool
{
    return m_showLabel1 != nullptr && m_showLabel1->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointShowLabel1(bool show)
{
    if (m_showLabel1 != nullptr)
    {
        m_showLabel1->setChecked(show);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointShowLabel2() const -> bool
{
    return m_showLabel2 != nullptr && m_showLabel2->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointShowLabel2(bool show)
{
    if (m_showLabel2 != nullptr)
    {
        m_showLabel2->setChecked(show);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::GetPointShowLabel3() const -> bool
{
    return m_showLabel3 != nullptr && m_showLabel3->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SetPointShowLabel3(bool show)
{
    if (m_showLabel3 != nullptr)
    {
        m_showLabel3->setChecked(show);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::ShowDialog(bool click)
{
    Q_UNUSED(click)
    // The dialog is already up: it opens on the release that chose the point, and the figure takes nothing else
    // from the canvas.
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxStartPoint->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
    ui->lineEditPoint1Name->setEnabled(ready);
    ui->lineEditPoint2Name->setEnabled(ready);
    ui->lineEditPoint3Name->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::ChosenObject(quint32 id, const SceneObject &type)
{
    qDebug() << "FLOUNCE-DEBUG: ChosenObject id =" << id << " type =" << static_cast<int>(type)
             << " prepare =" << prepare;

    if (prepare)
    {
        return; // After first choose we ignore all objects
    }

    if (type == SceneObject::Point && SetObject(id, ui->comboBoxStartPoint, QString()))
    {
        prepare = true;
        FinishCreating();
        qDebug() << "FLOUNCE-DEBUG: FinishCreating appele, visible =" << isVisible();
    }
    else
    {
        qDebug() << "FLOUNCE-DEBUG: point refuse (type pas Point, ou absent de la liste)";
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::UsesVisualization() const -> bool
{
    // There is nothing to preview: the figure is solved when it is created, not while it is typed.
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::SaveData()
{
    ApplyPointNames();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogFlounceSpiral::IsValid() const -> bool
{
    const bool widths = m_formulaWidth->isValid();
    const bool lengths = m_formulaLength->isValid();
    const bool angles = m_formulaAngle->isValid();
    return widths && lengths && angles && m_flagAlias && m_flagPoint1 && m_flagPoint2 && m_flagPoint3;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::ValidateAlias()
{
    // ONE name, so one thing to check: the curve is named the way every other curve of the pattern is, and the name
    // has to be free — unless it is the one the figure already carries, in which case it is its own name.
    const QRegularExpression rx(NameRegExp());
    VSplinePath splPath;
    splPath.SetAliasSuffix(GetOuterAlias());
    const QString alias = splPath.GetAlias();

    const bool aliasOk = GetOuterAlias().isEmpty()
                         || (rx.match(alias).hasMatch()
                             && (m_originAlias == GetOuterAlias() || data.IsUnique(alias)));

    m_flagAlias = aliasOk;
    ui->lineEditAlias->setStyleSheet(aliasOk ? QString() : QStringLiteral("border: 1px solid red;"));
    ChangeColor(ui->labelAlias, aliasOk ? OkColor(this) : errorColor);

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::ValidatePointNames()
{
    CheckPointLabel(this, ui->lineEditPoint1Name, ui->labelPoint1Name, GetPointName1(), &data, m_flagPoint1);
    CheckPointLabel(this, ui->lineEditPoint2Name, ui->labelPoint2Name, GetPointName2(), &data, m_flagPoint2);
    CheckPointLabel(this, ui->lineEditPoint3Name, ui->labelPoint3Name, GetPointName3(), &data, m_flagPoint3);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::FinishCreating()
{
    emit ToolTip(QString());
    setModal(true);
    show();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogFlounceSpiral::ApplyPointNames()
{
    // The names are read back by the tool from the dialog, so nothing has to be pushed here: the point itself is
    // renamed when the figure is rebuilt.
}
