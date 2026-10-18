/************************************************************************
 **
 **  @file   dialogshoulderpoint.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include "dialogshoulderpoint.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QToolButton>

#include "../../visualization/line/vistoolshoulderpoint.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../support/dialogeditwrongformula.h"
#include "../vgeometry/vpointf.h"
#include "vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogshoulderpoint.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogShoulderPoint create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogShoulderPoint::DialogShoulderPoint(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogShoulderPoint),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);
    connect(AddShowPointNameCheckbox(ui->formLayoutPoint), &QCheckBox::toggled, this,
            [this](bool checked) { SetShowLabel(checked); });

    InitIcons();

    // --- Configure formula widget ---
    m_formula->setData(&this->data);
    m_formula->setToolId(toolId);
    m_formula->setLabelText(tr("Length:"));
    m_formula->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    m_formula->setDialogTitle(tr("Edit length"));
    connect(m_formula, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    if (auto *placeholder = ui->formulaPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formula);
            placeholder->deleteLater();
        }
    }


    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    InitOkCancelApply(ui);

    FillComboBoxTypeLine(ui->comboBoxLineType, LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                                              ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    // Role name field (Line_CarrureDevant)
    m_labelRoleName = new QLabel(tr("Line:"));
    m_labelRoleName->setMinimumWidth(90);
    auto *lineEditRoleName = new QLineEdit();
    lineEditRoleName->setObjectName(QStringLiteral("lineEditRoleName"));
    auto *roleNameLayout = new QHBoxLayout;
    roleNameLayout->setContentsMargins(0, 0, 0, 0);
    roleNameLayout->addWidget(m_labelRoleName);
    roleNameLayout->addWidget(lineEditRoleName);
    ui->verticalLayout->insertLayout(0, roleNameLayout);
    connect(lineEditRoleName, &QLineEdit::textEdited, this, &DialogShoulderPoint::ValidateLineRoleName);

    // Separator between the line name field and the formula
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout->insertLayout(1, lineSeparatorLayout);

    vis = new VisToolShoulderPoint(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogShoulderPoint::IsValid() const -> bool
{
    return m_formula->isValid() && m_flagName && flagRoleName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::ShowVisualization()
{
    AddVisualization<VisToolShoulderPoint>();
}

//---------------------------------------------------------------------------------------------------------------------
DialogShoulderPoint::~DialogShoulderPoint()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogShoulderPoint::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogShoulderPoint::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare)
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
        SCASSERT(line != nullptr)

        switch (m_number)
        {
            case 0:
                m_p3 = id;
                emit ToolTip(tr("Select first point of line"));
                m_number++;
                line->VisualMode(id);
                break;
            case 1:
                if (m_p3 != id)
                {
                    m_p1Line = id;
                    emit ToolTip(tr("Select second point of line"));
                    m_number++;
                    line->SetLineP1Id(id);
                    line->RefreshGeometry();
                }
                break;
            case 2:
                ChosenThirdPoint(id);
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();
    const QString formulaText = VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
        m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator());

    auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
    SCASSERT(line != nullptr)

    line->SetPoint3Id(GetP3());
    line->SetLineP1Id(GetP1Line());
    line->SetLineP2Id(GetP2Line());
    line->SetLength(formulaText);
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::ChosenThirdPoint(quint32 id)
{
    if (m_p3 != id && m_p1Line != id)
    {
        m_p2Line = id;

        auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
        SCASSERT(window != nullptr)

        auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
        SCASSERT(line != nullptr)
        connect(line, &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

        line->SetLineP2Id(id);
        line->RefreshGeometry();
        prepare = true;

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();
    emit ToolTip(QString());
    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::InitIcons()
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setPShoulder set id shoulder point
 * @param value id
 */
void DialogShoulderPoint::SetP3(const quint32 &value)
{
    m_p3 = value;

    auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint3Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogShoulderPoint::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetP2Line set id second point of line
 * @param value id
 */
void DialogShoulderPoint::SetP2Line(const quint32 &value)
{
    m_p2Line = value;

    auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
    SCASSERT(line != nullptr)
    line->SetLineP2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetP1Line set id first point of line
 * @param value id
 */
void DialogShoulderPoint::SetP1Line(const quint32 &value)
{
    m_p1Line = value;

    auto *line = qobject_cast<VisToolShoulderPoint *>(vis);
    SCASSERT(line != nullptr)
    line->SetLineP1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFormula set string of formula
 * @param value formula
 */
void DialogShoulderPoint::SetFormula(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *line = qobject_cast<VisToolShoulderPoint *>(vis))
    {
        line->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetTypeLine set type of line
 * @param value type
 */
void DialogShoulderPoint::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPointName set name of point
 * @param value name
 */
void DialogShoulderPoint::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetTypeLine return type of line
 * @return type
 */
auto DialogShoulderPoint::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFormula return string of formula
 * @return formula
 */
auto DialogShoulderPoint::GetFormula() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetP1Line return id first point of line
 * @return id
 */
auto DialogShoulderPoint::GetP1Line() const -> quint32
{
    return m_p1Line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetP2Line return id second point of line
 * @return id
 */
auto DialogShoulderPoint::GetP2Line() const -> quint32
{
    return m_p2Line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getPShoulder return id shoulder point
 * @return id
 */
auto DialogShoulderPoint::GetP3() const -> quint32
{
    return m_p3;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogShoulderPoint::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::SetLineRoleName(const QString &suffix)
{
    m_initialRoleName = suffix;
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogShoulderPoint::GetLineRoleName() const -> QString
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::ValidateLineRoleName()
{
    auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName"));
    if (!edit)
    {
        return;
    }
    const QString suffix = edit->text().trimmed();
    if (suffix.isEmpty())
    {
        flagRoleName = true;
        if (edit) { edit->setStyleSheet({}); }
        ChangeColor(m_labelRoleName, OkColor(this));
        CheckState();
        return;
    }

    QRegularExpression const rx(NameRegExp());
    if (!rx.match(suffix).hasMatch())
    {
        flagRoleName = false;
        if (edit) { edit->setStyleSheet(QStringLiteral("border: 1px solid red;")); }
        ChangeColor(m_labelRoleName, errorColor);
        CheckState();
        return;
    }

    // A name designates one line: the pattern refuses a suffix another pair already holds. The variable map is
    // no witness here — its keys go through VarToUser (`Line_` becomes `Ligne_` in French), so a check written
    // against it passes in every language but English. The line's own name is not a clash with itself.
    // Both witnesses are asked: the role table (what the pattern loads and writes) and the name table (what the
    // tools registered), so the dialog never accepts what the setter would then refuse.
    const bool taken = data.IsLineRoleNameTaken(suffix) || not data.IsUnique(line_ + suffix);
    if (suffix != m_initialRoleName && taken)
    {
        flagRoleName = false;
        if (edit) { edit->setStyleSheet(QStringLiteral("border: 1px solid red;")); }
        ChangeColor(m_labelRoleName, errorColor);
        CheckState();
        return;
    }
    flagRoleName = true;
    if (edit) { edit->setStyleSheet({}); }
    ChangeColor(m_labelRoleName, OkColor(this));
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::ShowDialog(bool click)
{
    if (not prepare)
    {
        return;
    }

    if (click)
    {
        // The check need to ignore first release of mouse button.
        // User can select point by clicking on a label.
        if (not m_firstRelease)
        {
            m_firstRelease = true;
            return;
        }

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)

        const QSharedPointer<VPointF> p3 = data.GeometricObject<VPointF>(GetP3());
        QLineF const line(static_cast<QPointF>(*p3), scene->getScenePos());
        SetFormula(QString::number(RoundForDisplay(FromPixel(line.length(), *data.GetPatternUnit()))));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogShoulderPoint::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
}
