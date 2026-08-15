/************************************************************************
 **
 **  @file   dialognormal.cpp
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

#include "dialognormal.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointF>
#include <QRegularExpression>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>

#include "../../visualization/line/vistoolnormal.h"
#include "../../visualization/visualization.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "vtheme.h"
#include "ui_dialognormal.h"

//---------------------------------------------------------------------------------------------------------------------
DialogNormal::DialogNormal(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogNormal),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    // Re-add spanning 2 rows
    ui->gridLayoutPoints->removeItem(ui->gridLayoutPoints->itemAtPosition(0, 2));
    ui->gridLayoutPoints->addLayout(ui->verticalLayoutSwap, 0, 2, 2, 1);

    MakeFilterableComboBox(ui->comboBoxFirstPoint);
    MakeFilterableComboBox(ui->comboBoxSecondPoint);

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

    ui->lineEditNamePoint->setClearButtonEnabled(true);
    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxFirstPoint);
    FillComboBoxPoints(ui->comboBoxSecondPoint);
    FillComboBoxTypeLine(ui->comboBoxLineType, LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                                              ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    connect(ui->toolButtonArrowDown, &QPushButton::clicked, this, [this]() { ui->doubleSpinBoxAngle->setValue(270); });
    connect(ui->toolButtonArrowUp, &QPushButton::clicked, this, [this]() { ui->doubleSpinBoxAngle->setValue(90); });
    connect(ui->toolButtonArrowLeft, &QPushButton::clicked, this, [this]() { ui->doubleSpinBoxAngle->setValue(180); });
    connect(ui->toolButtonArrowRight, &QPushButton::clicked, this, [this]() { ui->doubleSpinBoxAngle->setValue(0); });
    connect(ui->toolButtonArrowLeftUp, &QPushButton::clicked, this,
            [this]() { ui->doubleSpinBoxAngle->setValue(135); });
    connect(ui->toolButtonArrowLeftDown, &QPushButton::clicked, this,
            [this]() { ui->doubleSpinBoxAngle->setValue(225); });
    connect(ui->toolButtonArrowRightUp, &QPushButton::clicked, this,
            [this]() { ui->doubleSpinBoxAngle->setValue(45); });
    connect(ui->toolButtonArrowRightDown, &QPushButton::clicked, this,
            [this]() { ui->doubleSpinBoxAngle->setValue(315); });

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxFirstPoint, &QComboBox::currentTextChanged, this, &DialogNormal::PointNameChanged);
    connect(ui->comboBoxSecondPoint, &QComboBox::currentTextChanged, this, &DialogNormal::PointNameChanged);
    connect(ui->toolButtonSwapPoints, &QToolButton::clicked, this, &DialogNormal::SwapPoints);

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
    connect(lineEditRoleName, &QLineEdit::textEdited, this, &DialogNormal::ValidateLineRoleName);

    // Separator between the line name field and the formula
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout->insertLayout(1, lineSeparatorLayout);

    vis = new VisToolNormal(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::IsValid() const -> bool
{
    return m_flagName && m_formula->isValid() && m_flagError && flagRoleName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::PointNameChanged()
{
    QColor color;
    if (getCurrentObjectId(ui->comboBoxFirstPoint) == getCurrentObjectId(ui->comboBoxSecondPoint))
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    ChangeColor(ui->labelFirstPoint, color);
    ChangeColor(ui->labelSecondPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::ShowVisualization()
{
    AddVisualization<VisToolNormal>();
}

//---------------------------------------------------------------------------------------------------------------------
DialogNormal::~DialogNormal()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare)
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *line = qobject_cast<VisToolNormal *>(vis);
        SCASSERT(line != nullptr)

        switch (m_number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxFirstPoint, tr("Select second point of line")))
                {
                    m_number++;
                    line->VisualMode(id);
                }
                break;
            case 1:
                if (getCurrentObjectId(ui->comboBoxFirstPoint) != id &&
                    SetObject(id, ui->comboBoxSecondPoint, QString()))
                {
                    line->SetPoint2Id(id);
                    line->RefreshGeometry();
                    prepare = true;

                    auto *window =
                        qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
                    SCASSERT(window != nullptr)
                    connect(line, &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

                    if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
                    {
                        FinishCreating();
                    }
                }
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();
    m_angle = ui->doubleSpinBoxAngle->value();

    const QString formulaText = m_formula->formula();

    auto *line = qobject_cast<VisToolNormal *>(vis);
    SCASSERT(line != nullptr)

    line->SetPoint1Id(GetFirstPointId());
    line->SetPoint2Id(GetSecondPointId());
    line->SetLength(formulaText);
    line->SetAngle(m_angle);
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::changeEvent(QEvent *event)
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
void DialogNormal::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();
    emit ToolTip(QString());
    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::InitIcons()
{
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetSecondPointId(quint32 value)
{
    setCurrentPointId(ui->comboBoxSecondPoint, value);

    auto *line = qobject_cast<VisToolNormal *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetFirstPointId(quint32 value)
{
    setCurrentPointId(ui->comboBoxFirstPoint, value);

    auto *line = qobject_cast<VisToolNormal *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetAngle(qreal value)
{
    m_angle = value;
    ui->doubleSpinBoxAngle->setValue(m_angle);

    auto *line = qobject_cast<VisToolNormal *>(vis);
    SCASSERT(line != nullptr)
    line->SetAngle(m_angle);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetFormula(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *line = qobject_cast<VisToolNormal *>(vis))
    {
        line->SetLength(m_formula->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetFormula() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetAngle() const -> qreal
{
    return m_angle;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetFirstPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxFirstPoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetSecondPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxSecondPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::ShowDialog(bool click)
{
    if (not prepare)
    {
        return;
    }

    if (click)
    {
        if (not m_firstRelease)
        {
            m_firstRelease = true;
            return;
        }

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)

        const QSharedPointer<VPointF> p1 = data.GeometricObject<VPointF>(GetFirstPointId());
        const QSharedPointer<VPointF> p2 = data.GeometricObject<VPointF>(GetSecondPointId());
        QLineF baseLine(static_cast<QPointF>(*p1), static_cast<QPointF>(*p2));
        baseLine.setAngle(baseLine.angle() + 90);

        QLineF const line(static_cast<QPointF>(*p1), scene->getScenePos());

        qreal len = line.length();
        qreal const angleTo = baseLine.angleTo(line);
        if (angleTo > 90 && angleTo < 270)
        {
            len *= -1;
        }

        SetFormula(QString::number(FromPixel(len, *data.GetPatternUnit())));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SwapPoints()
{
    const quint32 first = GetFirstPointId();
    const quint32 second = GetSecondPointId();
    SetFirstPointId(second);
    SetSecondPointId(first);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
    ui->comboBoxFirstPoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::SetLineRoleName(const QString &suffix)
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNormal::GetLineRoleName() const -> QString
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNormal::ValidateLineRoleName()
{
    auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName"));
    if (edit == nullptr)
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
    // Uniqueness: Line_<suffix> must not exist on another line
    const QString aliasKey = line_ + suffix;
    if (data.DataLengthLines().contains(aliasKey))
    {
        const QSharedPointer<VLengthLine> var = data.DataLengthLines().value(aliasKey);
        const QVector<quint32> refs = var->GetReferences();
        if (refs.size() >= 2 && refs.at(0) != NULL_ID && refs.at(1) != NULL_ID)
        {
            flagRoleName = false;
            if (edit) { edit->setStyleSheet(QStringLiteral("border: 1px solid red;")); }
            ChangeColor(m_labelRoleName, errorColor);
            CheckState();
            return;
        }
    }
    flagRoleName = true;
    if (edit) { edit->setStyleSheet({}); }
    ChangeColor(m_labelRoleName, OkColor(this));
    CheckState();
}
