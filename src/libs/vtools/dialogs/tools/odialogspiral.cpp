/************************************************************************
 **
 **  @file   odialogspiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Dialog of the Spiral tool.
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

#include "odialogspiral.h"

#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QTimer>

#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/angleformulawidget.h"
#include "../widgets/formulainputwidget.h"
#include "../../tools/drawTools/toolpoint/toolsinglepoint/vtoolsinglepoint.h"
#include "ui_odialogspiral.h"

//---------------------------------------------------------------------------------------------------------------------
ODialogSpiral::ODialogSpiral(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::ODialogSpiral),
    m_formulaRadius(new FormulaInputWidget(this)),
    m_formulaLength(new FormulaInputWidget(this)),
    m_formulaAngle(new AngleFormulaWidget(this))
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxStartPoint);

    // --- Configure formula widgets ---
    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaRadius->setData(&this->data);
    m_formulaRadius->setToolId(toolId);
    m_formulaRadius->setLabelText(tr("Radius:"));
    m_formulaRadius->setPostfix(lengthPostfix);
    m_formulaRadius->setDialogTitle(tr("Edit radius"));
    m_formulaRadius->setCheckZero(true);
    m_formulaRadius->setCheckLessThanZero(true);
    connect(m_formulaRadius, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

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
    // A spiral with no rotation is the common case, so the neutral value is already there.
    m_formulaAngle->setFormula(QStringLiteral("0"));

    // Replace formula placeholders in the UI layout with the actual widgets.
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
    replacePlaceholder(ui->formulaRadiusPlaceholder, m_formulaRadius);
    replacePlaceholder(ui->formulaLengthPlaceholder, m_formulaLength);
    replacePlaceholder(ui->formulaAnglePlaceholder, m_formulaAngle);

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxStartPoint);

    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &ODialogSpiral::ValidateAlias);
    connect(ui->lineEditEndPointName, &QLineEdit::textEdited, this, &ODialogSpiral::ValidateEndPointName);

    // Move the curve-name field (Alias) to the top of the Tool tab, above the hint and the formulas
    ui->labelAlias->setMinimumWidth(90);

    auto aliasRow = ui->formLayout->takeRow(3);
    QWidget *aliasLabel = aliasRow.labelItem->widget();
    QWidget *aliasField = aliasRow.fieldItem->widget();
    delete aliasRow.labelItem;
    delete aliasRow.fieldItem;

    auto *aliasLayout = new QHBoxLayout;
    aliasLayout->setContentsMargins(0, 0, 0, 0);
    aliasLayout->addWidget(aliasLabel);
    aliasLayout->addWidget(aliasField);
    ui->verticalLayout->insertLayout(0, aliasLayout);

    // Separator between the curve-name field and what follows it
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout->insertLayout(1, lineSeparatorLayout);

    // The name of the arrival point, always created, and whether its name is shown on canvas.
    m_endShowLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutEndPoint);
    connect(m_endShowLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { m_showLabelEnd = checked; });

    ui->lineEditEndPointName->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
    m_originEndPointName = ui->lineEditEndPointName->text();
    ValidateEndPointName();

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogSpiral::~ODialogSpiral()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetStartPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxStartPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetStartPointId(quint32 id)
{
    ChangeCurrentData(ui->comboBoxStartPoint, id);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetFormulaRadius() const -> QString
{
    return m_formulaRadius->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetFormulaRadius(const QString &value)
{
    m_formulaRadius->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetFormulaLength() const -> QString
{
    return m_formulaLength->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetFormulaLength(const QString &value)
{
    m_formulaLength->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetFormulaAngle() const -> QString
{
    return m_formulaAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetFormulaAngle(const QString &value)
{
    m_formulaAngle->setFormula(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetEndPointName() const -> QString
{
    return ui->lineEditEndPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetEndPointName(const QString &name)
{
    m_originEndPointName = name;
    ui->lineEditEndPointName->setText(name);
    ValidateEndPointName();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetEndPointId(quint32 id)
{
    m_endPointId = id;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::GetEndShowLabel() const -> bool
{
    return m_showLabelEnd;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SetEndShowLabel(bool show)
{
    if (m_endShowLabelCheckbox != nullptr)
    {
        m_endShowLabelCheckbox->setChecked(show);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::ShowDialog(bool click)
{
    Q_UNUSED(click)
    // The dialog is already up: it opens on the release that chose the point, and the spiral takes nothing else
    // from the canvas — its length is a value, not a drag.
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxStartPoint->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
    ui->lineEditEndPointName->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Point && SetObject(id, ui->comboBoxStartPoint, QString()))
    {
        prepare = true;

        FinishCreating();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::IsValid() const -> bool
{
    return m_formulaRadius->isValid() && m_formulaLength->isValid() && m_formulaAngle->isValid() && m_flagAlias
           && m_flagEndPointName;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpiral::UsesVisualization() const -> bool
{
    // Nothing to preview: the spiral is a value, not a gesture. Its length is only solved once the tool is
    // created, so a visualization would have nothing to draw while the dialog is open.
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::SaveData()
{
    ApplyEndPointName();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::ApplyEndPointName()
{
    if (m_endPointId == NULL_ID || GetEndPointName() == m_originEndPointName)
    {
        return;
    }

    // The arrival point is a tool of its own. Renaming it goes through the same call the property browser uses,
    // so the rename is recorded, formulas follow, and the native name of the curve follows with them.
    if (auto *point = qobject_cast<VToolSinglePoint *>(VAbstractPattern::FindTool(m_endPointId)))
    {
        point->setName(GetEndPointName());
        m_originEndPointName = GetEndPointName();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::ValidateAlias()
{
    VSplinePath splPath;
    splPath.SetAliasSuffix(GetAliasSuffix());
    if (QRegularExpression const rx(NameRegExp());
        not GetAliasSuffix().isEmpty()
        && (not rx.match(splPath.GetAlias()).hasMatch()
            || (m_originAliasSuffix != GetAliasSuffix() && not data.IsUnique(splPath.GetAlias()))))
    {
        m_flagAlias = false;
        ui->lineEditAlias->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias, errorColor);
    }
    else
    {
        m_flagAlias = true;
        ui->lineEditAlias->setStyleSheet({});
        ChangeColor(ui->labelAlias, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::ValidateEndPointName()
{
    const QString name = GetEndPointName();
    const bool renamed = name != m_originEndPointName;

    // The current name belongs to the point already: it is only a duplicate when it is a new one.
    if (QRegularExpression const rx(NameRegExp());
        name.isEmpty() || not rx.match(name).hasMatch() || (renamed && not data.IsUnique(name)))
    {
        m_flagEndPointName = false;
        ui->lineEditEndPointName->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelEndPointName, errorColor);
    }
    else
    {
        m_flagEndPointName = true;
        ui->lineEditEndPointName->setStyleSheet({});
        ChangeColor(ui->labelEndPointName, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpiral::FinishCreating()
{
    emit ToolTip(QString());

    setModal(true);
    show();

    CheckState();
}
