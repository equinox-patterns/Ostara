/************************************************************************
 **
 **  @file   odialogrectangle.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  the window that describes a rectangle
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

#include "odialogrectangle.h"
#include "ui_odialogrectangle.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../../visualization/line/ovistoolrectangle.h"
#include "../../visualization/visualization.h"
#include "../dialogtoolbox.h"
#include "../widgets/formulainputwidget.h"

//---------------------------------------------------------------------------------------------------------------------
ODialogRectangle::ODialogRectangle(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::ODialogRectangle),
    m_formulaWidth(new FormulaInputWidget(this)),
    m_formulaHeight(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    // The base point is the one the user clicked, and the preview is where it is seen. It is still held in a
    // combo, because validating a picked object goes through one — but that combo is never shown, and no name
    // of a point ever reaches the screen from here.
    m_comboBasePoint = MakeComboBox(this);
    m_comboBasePoint->setVisible(false);
    FillComboBoxPoints(m_comboBasePoint);

    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaWidth->setData(&this->data);
    m_formulaWidth->setToolId(toolId);
    m_formulaWidth->setLabelText(tr("Width:"));
    m_formulaWidth->setPostfix(postfix);
    m_formulaWidth->setDialogTitle(tr("Edit width"));
    connect(m_formulaWidth, &FormulaInputWidget::valueChanged, this,
            [this]()
            {
                RefreshPreview();
                CheckState();
            });

    m_formulaHeight->setData(&this->data);
    m_formulaHeight->setToolId(toolId);
    m_formulaHeight->setLabelText(tr("Height:"));
    m_formulaHeight->setPostfix(postfix);
    m_formulaHeight->setDialogTitle(tr("Edit height"));
    connect(m_formulaHeight, &FormulaInputWidget::valueChanged, this,
            [this]()
            {
                RefreshPreview();
                CheckState();
            });

    // The two formula blocks replace the placeholders the .ui keeps for them, exactly as every other tool
    // does: the block owns its label, its evaluated value, its field, its grow button and its fx button.
    if (auto *placeholder = ui->formulaWidthPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaWidth);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formulaHeightPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaHeight);
            placeholder->deleteLater();
        }
    }

    // The three points are named after the piece they belong to, and each of the three is offered a name and a
    // box of its own. GenerateLabel always restarts at 1, so each name is reserved as the next one is drawn.
    auto *pattern = VAbstractValApplication::VApp()->getCurrentDocument();
    const QString defaultNameB = pattern->GenerateLabel(LabelType::NewLabel);
    const QString defaultNameC = pattern->GenerateLabel(LabelType::NewLabel, defaultNameB);
    QString defaultNameD;
    {
        static const QRegularExpression reSuffix(QStringLiteral("^(.*?)(\\d+)$"));
        const QRegularExpressionMatch m = reSuffix.match(defaultNameC);
        if (m.hasMatch())
        {
            const QString base = m.captured(1);
            int num = m.captured(2).toInt() + 1;
            do
            {
                defaultNameD = base + QString::number(num++);
            } while (not data->IsUnique(defaultNameD) || defaultNameD == defaultNameB);
        }
        else
        {
            defaultNameD = pattern->GenerateLabel(LabelType::NewLabel, defaultNameC);
        }
    }

    ui->lineEditNameB->setText(defaultNameB);
    ui->lineEditNameC->setText(defaultNameC);
    ui->lineEditNameD->setText(defaultNameD);

    // Nothing is shown unless it is asked for: Ostara never draws a point name by default.
    ui->checkBoxShowB->setChecked(false);
    ui->checkBoxShowC->setChecked(false);
    ui->checkBoxShowD->setChecked(false);

    FillComboBoxTypeLine(ui->comboBoxLineType,
                         LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                        ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    InitIcons();

    connect(ui->lineEditNameB, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckOnePointName(ui->lineEditNameB, ui->labelPointB, m_flagNameB, m_originalNameB);
                CheckState();
            });
    connect(ui->lineEditNameC, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckOnePointName(ui->lineEditNameC, ui->labelPointC, m_flagNameC, m_originalNameC);
                CheckState();
            });
    connect(ui->lineEditNameD, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckOnePointName(ui->lineEditNameD, ui->labelPointD, m_flagNameD, m_originalNameD);
                CheckState();
            });

    connect(ui->lineEditWidthName, &QLineEdit::textEdited, this,
            [this]()
            {
                CheckOneLineRoleName(ui->lineEditWidthName, ui->labelWidthLine, m_flagWidthLineRoleName);
                CheckState();
            });
    connect(ui->lineEditHeightName, &QLineEdit::textEdited, this,
            [this]()
            {
                CheckOneLineRoleName(ui->lineEditHeightName, ui->labelHeightLine, m_flagHeightLineRoleName);
                CheckState();
            });

    // A change of type or of colour is seen at once, in the preview.
    connect(ui->comboBoxLineType, &QComboBox::currentIndexChanged, this, [this]() { RefreshPreview(); });
    connect(m_colorCombo, &QComboBox::currentIndexChanged, this, [this]() { RefreshPreview(); });

    InitOkCancelApply(ui);

    QWidget::setTabOrder(m_formulaWidth->editor(), ui->lineEditWidthName);
    QWidget::setTabOrder(ui->lineEditWidthName, m_formulaHeight->editor());
    QWidget::setTabOrder(m_formulaHeight->editor(), ui->lineEditHeightName);
    QWidget::setTabOrder(ui->lineEditHeightName, ui->comboBoxLineType);
    QWidget::setTabOrder(ui->comboBoxLineType, m_colorCombo);
    QWidget::setTabOrder(m_colorCombo, ui->plainTextEditToolNotes);
    QWidget::setTabOrder(ui->plainTextEditToolNotes, ui->lineEditNameB);
    QWidget::setTabOrder(ui->lineEditNameB, ui->checkBoxShowB);
    QWidget::setTabOrder(ui->checkBoxShowB, ui->lineEditNameC);
    QWidget::setTabOrder(ui->lineEditNameC, ui->checkBoxShowC);
    QWidget::setTabOrder(ui->checkBoxShowC, ui->lineEditNameD);
    QWidget::setTabOrder(ui->checkBoxShowD, ui->buttonBox);

    vis = new OVisToolRectangle(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);

    // The three names start out valid: they were just generated and are unique by construction.
    CheckOnePointName(ui->lineEditNameB, ui->labelPointB, m_flagNameB, m_originalNameB);
    CheckOnePointName(ui->lineEditNameC, ui->labelPointC, m_flagNameC, m_originalNameC);
    CheckOnePointName(ui->lineEditNameD, ui->labelPointD, m_flagNameD, m_originalNameD);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogRectangle::~ODialogRectangle()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::InitIcons()
{
    // The same two colours the preview draws the two lengths with — the same pair, so that a dot always means
    // the same side on screen and in the window.
    ui->labelWidthDot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor2), 10));
    ui->labelHeightDot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor4), 10));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetBasePointId() const -> quint32
{
    return getCurrentObjectId(m_comboBasePoint);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetBasePointId(quint32 value)
{
    setCurrentPointId(m_comboBasePoint, value);
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetFormulaWidth() const -> QString
{
    return m_formulaWidth->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetFormulaWidth(const QString &value)
{
    m_formulaWidth->setFormula(value);
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetFormulaHeight() const -> QString
{
    return m_formulaHeight->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetFormulaHeight(const QString &value)
{
    m_formulaHeight->setFormula(value);
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetPointBName() const -> QString
{
    return ui->lineEditNameB->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetPointBName(const QString &value)
{
    if (m_originalNameB.isEmpty())
    {
        m_originalNameB = value;
    }
    ui->lineEditNameB->setText(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetPointCName() const -> QString
{
    return ui->lineEditNameC->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetPointCName(const QString &value)
{
    if (m_originalNameC.isEmpty())
    {
        m_originalNameC = value;
    }
    ui->lineEditNameC->setText(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetPointDName() const -> QString
{
    return ui->lineEditNameD->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetPointDName(const QString &value)
{
    if (m_originalNameD.isEmpty())
    {
        m_originalNameD = value;
    }
    ui->lineEditNameD->setText(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::IsPointBLabelVisible() const -> bool
{
    return ui->checkBoxShowB->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::IsPointCLabelVisible() const -> bool
{
    return ui->checkBoxShowC->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::IsPointDLabelVisible() const -> bool
{
    return ui->checkBoxShowD->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetShowPointLabels(bool showB, bool showC, bool showD)
{
    ui->checkBoxShowB->setChecked(showB);
    ui->checkBoxShowC->setChecked(showC);
    ui->checkBoxShowD->setChecked(showD);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetWidthLineRoleName() const -> QString
{
    return ui->lineEditWidthName->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetWidthLineRoleName(const QString &value)
{
    ui->lineEditWidthName->setText(value);
    m_lineNamesInUse.insert(ui->lineEditWidthName, value.trimmed());
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetHeightLineRoleName() const -> QString
{
    return ui->lineEditHeightName->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetHeightLineRoleName(const QString &value)
{
    ui->lineEditHeightName->setText(value);
    m_lineNamesInUse.insert(ui->lineEditHeightName, value.trimmed());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogRectangle::IsValid() const -> bool
{
    return GetBasePointId() != NULL_ID && m_formulaWidth->isValid() && m_formulaHeight->isValid() && m_flagNameB
           && m_flagNameC && m_flagNameD && m_flagWidthLineRoleName && m_flagHeightLineRoleName;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::CheckDependencyTreeComplete()
{
    // A rectangle depends on one existing point, and nothing it creates can feed it back: no graph to check.
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::ShowDialog(bool click)
{
    if (not prepare || not click)
    {
        return;
    }

    setModal(true);
    emit ToolTip(QString());
    show();
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After the first pick every other object is ignored.
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        if (SetObject(id, m_comboBasePoint, QString()))
        {
            if (not vis.isNull())
            {
                vis->VisualMode(id);
                auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
                SCASSERT(window != nullptr)
                connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
            }
            prepare = true;
            RefreshPreview();
            emit ToolTip(tr("Set the width and the height, then click OK"));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::ShowVisualization()
{
    AddVisualization<OVisToolRectangle>();
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::RefreshPreview()
{
    auto *visual = qobject_cast<OVisToolRectangle *>(vis);
    if (visual == nullptr)
    {
        return;
    }

    visual->SetBasePointId(GetBasePointId());
    visual->SetFormulaWidth(GetFormulaWidth());
    visual->SetFormulaHeight(GetFormulaHeight());
    visual->SetTypeLine(GetTypeLine());
    visual->SetLineColor(GetLineColor());
    visual->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::SaveData()
{
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    else if (event->type() == QEvent::PaletteChange)
    {
        // The two pastilles are painted, so a change of theme has to repaint them.
        InitIcons();
    }
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::CheckOnePointName(QLineEdit *edit, QLabel *label, bool &flag, const QString &originalName)
{
    SCASSERT(edit != nullptr)
    SCASSERT(label != nullptr)

    const QString name = edit->text();

    // When the window edits an existing rectangle, the name it started with is already in the container and
    // therefore not unique — it is accepted for this field, and only for this field.
    const bool isOriginal = (not originalName.isEmpty() && name == originalName);

    if (QRegularExpression const rx(NameRegExp());
        name.isEmpty() || not rx.match(name).hasMatch() || (not isOriginal && not data.IsUnique(name)))
    {
        flag = false;
        ChangeColor(label, errorColor);
    }
    else
    {
        flag = true;
        ChangeColor(label, OkColor(this));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogRectangle::CheckOneLineRoleName(QLineEdit *edit, QLabel *label, bool &flag)
{
    SCASSERT(edit != nullptr)
    SCASSERT(label != nullptr)

    const QString suffix = edit->text().trimmed();
    const QString currentSuffix = m_lineNamesInUse.contains(edit) ? m_lineNamesInUse.value(edit) : QString();

    // An empty box is valid: it only means the length stays anonymous, with no variable to read it by.
    if (suffix.isEmpty())
    {
        flag = true;
        edit->setStyleSheet({});
        ChangeColor(label, OkColor(this));
        return;
    }

    // The shape is the one every role name of a pattern follows, and the name must not already label another
    // line. The name this very row started with is already in the container, so it is accepted for it.
    const bool free = (suffix == currentSuffix) || data.IsUnique(line_ + suffix);

    if ((not QRegularExpression(NameRegExp()).match(suffix).hasMatch()) || not free)
    {
        flag = false;
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(label, errorColor);
    }
    else
    {
        flag = true;
        edit->setStyleSheet({});
        ChangeColor(label, OkColor(this));
    }
}
