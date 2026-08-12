/************************************************************************
 **
 **  @file   dialogalongline.cpp
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

#include "dialogalongline.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSharedPointer>
#include <QTimer>
#include <QToolButton>

#include "../../dialogs/dialogtoolbox.h"
#include "../../visualization/line/vistoolalongline.h"
#include "../../visualization/visualization.h"
#include "../../../../vtheme/vscenestylesheet.h"
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
#include "vtheme.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogalongline.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
DialogAlongLine::DialogAlongLine(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogAlongLine),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    InitIcons();

    // Re-add spanning 2 rows
    ui->gridLayout->removeItem(ui->gridLayout->itemAtPosition(1, 2));
    ui->gridLayout->addLayout(ui->verticalLayoutSwap, 1, 2, 2, 1);

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

    // Replace formula placeholder with actual widget
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

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxFirstPoint, &QComboBox::currentTextChanged, this, &DialogAlongLine::PointChanged);
    connect(ui->comboBoxSecondPoint, &QComboBox::currentTextChanged, this, &DialogAlongLine::PointChanged);
    connect(ui->toolButtonSwapPoints, &QToolButton::clicked, this, &DialogAlongLine::SwapPoints);

    // Role name field A→C
    auto *containerRoleName = new QWidget();
    auto *layoutRoleName = new QHBoxLayout(containerRoleName);
    layoutRoleName->setContentsMargins(0, 0, 0, 0);
    layoutRoleName->setSpacing(4);
    auto *labelRoleNameDot = new QLabel();
    labelRoleNameDot->setObjectName(QStringLiteral("labelRoleNameDot"));
    labelRoleNameDot->setFixedSize(12, 12);
    m_labelRoleName = new QLabel(tr("Name:"));
    m_labelRoleName->setMinimumWidth(80);
    layoutRoleName->addWidget(labelRoleNameDot);
    layoutRoleName->addWidget(m_labelRoleName);
    layoutRoleName->addStretch();
    auto *lineEditRoleName = new QLineEdit();
    lineEditRoleName->setObjectName(QStringLiteral("lineEditRoleName"));
    ui->gridLayout->addWidget(containerRoleName, 5, 0);
    ui->gridLayout->addWidget(lineEditRoleName, 5, 1);
    connect(lineEditRoleName, &QLineEdit::textEdited, this, &DialogAlongLine::ValidateLineRoleName);

    // Role name field C→B
    auto *containerSecondRoleName = new QWidget();
    auto *layoutSecondRoleName = new QHBoxLayout(containerSecondRoleName);
    layoutSecondRoleName->setContentsMargins(0, 0, 0, 0);
    layoutSecondRoleName->setSpacing(4);
    auto *labelSecondRoleNameDot = new QLabel();
    labelSecondRoleNameDot->setObjectName(QStringLiteral("labelSecondRoleNameDot"));
    labelSecondRoleNameDot->setFixedSize(12, 12);
    m_labelSecondRoleName = new QLabel(tr("Name 2:"));
    m_labelSecondRoleName->setMinimumWidth(80);
    layoutSecondRoleName->addWidget(labelSecondRoleNameDot);
    layoutSecondRoleName->addWidget(m_labelSecondRoleName);
    layoutSecondRoleName->addStretch();
    auto *lineEditSecondRoleName = new QLineEdit();
    lineEditSecondRoleName->setObjectName(QStringLiteral("lineEditSecondRoleName"));
    ui->gridLayout->addWidget(containerSecondRoleName, 6, 0);
    ui->gridLayout->addWidget(lineEditSecondRoleName, 6, 1);
    connect(lineEditSecondRoleName, &QLineEdit::textEdited, this, &DialogAlongLine::ValidateSecondLineRoleName);

    vis = new VisToolAlongLine(&this->data);

    // Call after initialization vis!!!!
    SetTypeLine(TypeLineNone); // By default don't show line

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::IsValid() const -> bool
{
    return m_flagName && m_formula->isValid() && m_flagError && flagRoleName && flagSecondRoleName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::PointChanged()
{
    QColor color;
    if (GetFirstPointId() == GetSecondPointId())
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    SetCurrentLength();
    ChangeColor(ui->labelFirstPoint, color);
    ChangeColor(ui->labelSecondPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::RestoreTransientVariables()
{
    SetCurrentLength();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ShowVisualization()
{
    AddVisualization<VisToolAlongLine>();
}

//---------------------------------------------------------------------------------------------------------------------
DialogAlongLine::~DialogAlongLine()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *line = qobject_cast<VisToolAlongLine *>(vis);
        SCASSERT(line != nullptr)

        const QString toolTip = tr("Select second point of line");
        switch (m_number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxFirstPoint, toolTip))
                {
                    m_number++;
                    line->VisualMode(id);
                }
                break;
            case 1:
                ChosenSecondPoint(id, toolTip);
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();

    const QString formulaText = m_formula->formula();

    auto *line = qobject_cast<VisToolAlongLine *>(vis);
    SCASSERT(line != nullptr)

    line->SetPoint1Id(GetFirstPointId());
    line->SetPoint2Id(GetSecondPointId());
    line->SetLength(formulaText);
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons();
        InitDialogButtonBoxIcons(ui->buttonBox);
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetCurrentLength()
{
    VLengthLine *length = nullptr;
    try
    {
        const QSharedPointer<VPointF> p1 = data.GeometricObject<VPointF>(GetFirstPointId());
        const QSharedPointer<VPointF> p2 = data.GeometricObject<VPointF>(GetSecondPointId());

        length = new VLengthLine(p1.data(), GetFirstPointId(), p2.data(), GetSecondPointId(), *data.GetPatternUnit());
    }
    catch (const VExceptionBadId &)
    {
        QScopedPointer<VPointF> const p1(new VPointF());
        QScopedPointer<VPointF> const p2(new VPointF());
        length = new VLengthLine(p1.data(), GetFirstPointId(), p2.data(), GetSecondPointId(), *data.GetPatternUnit());
    }

    SCASSERT(length != nullptr)
    length->SetName(currentLength);

    auto *locData = const_cast<VContainer *>(&data);
    locData->AddVariable(length);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ChosenSecondPoint(quint32 id, const QString &toolTip)
{
    if (SetObject(id, ui->comboBoxSecondPoint, QString()))
    {
        auto *line = qobject_cast<VisToolAlongLine *>(vis);
        SCASSERT(line != nullptr)

        if (m_flagError)
        {
            line->SetPoint2Id(id);
            if (m_buildMidpoint)
            {
                SetFormula(currentLength + "/2"_L1);
                line->SetMode(Mode::Show);
            }
            else
            {
                auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
                SCASSERT(window != nullptr)
                connect(line, &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
            }

            line->RefreshGeometry();

            prepare = true;

            if (m_buildMidpoint || not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
            {
                FinishCreating();
            }
        }
        else
        {
            emit ToolTip(toolTip);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();
    emit ToolTip(QString());
    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::InitIcons()
{
    if (auto *dot = findChild<QLabel *>(QStringLiteral("labelRoleNameDot")))
    {
        dot->setPixmap(
            GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor2), 10));
    }
    if (auto *dot = findChild<QLabel *>(QStringLiteral("labelSecondRoleNameDot")))
    {
        dot->setPixmap(
            GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor4), 10));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetSecondPointId(quint32 value)
{
    setCurrentPointId(ui->comboBoxSecondPoint, value);

    auto *line = qobject_cast<VisToolAlongLine *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::Build(const Tool &type)
{
    if (type == Tool::Midpoint)
    {
        m_buildMidpoint = true;
        auto *line = qobject_cast<VisToolAlongLine *>(vis);
        SCASSERT(line != nullptr)
        line->SetMidPointMode(m_buildMidpoint);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ShowDialog(bool click)
{
    if (not prepare || m_buildMidpoint)
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
        QLineF const baseLine(static_cast<QPointF>(*p1), static_cast<QPointF>(*p2));

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
void DialogAlongLine::SwapPoints()
{
    const quint32 first = GetFirstPointId();
    const quint32 second = GetSecondPointId();
    SetFirstPointId(second);
    SetSecondPointId(first);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
    ui->comboBoxFirstPoint->setEnabled(ready);
    ui->comboBoxSecondPoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetFirstPointId(quint32 value)
{
    setCurrentPointId(ui->comboBoxFirstPoint, value);

    auto *line = qobject_cast<VisToolAlongLine *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetFormula(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *line = qobject_cast<VisToolAlongLine *>(vis))
    {
        line->SetLength(m_formula->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetFormula() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetFirstPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxFirstPoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetSecondPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxSecondPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetLineRoleName(const QString &suffix)
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetLineRoleName() const -> QString
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ValidateLineRoleName()
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

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetSecondLineRoleName(const QString &suffix)
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditSecondRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetSecondLineRoleName() const -> QString
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditSecondRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ValidateSecondLineRoleName()
{
    auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditSecondRoleName"));
    if (!edit)
    {
        return;
    }
    const QString suffix = edit->text().trimmed();
    if (suffix.isEmpty())
    {
        flagSecondRoleName = true;
        edit->setStyleSheet({});
        ChangeColor(m_labelSecondRoleName, OkColor(this));
        CheckState();
        return;
    }

    QRegularExpression const rx(NameRegExp());
    if (!rx.match(suffix).hasMatch())
    {
        flagSecondRoleName = false;
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(m_labelSecondRoleName, errorColor);
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
            flagSecondRoleName = false;
            edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
            ChangeColor(m_labelSecondRoleName, errorColor);
            CheckState();
            return;
        }
    }
    flagSecondRoleName = true;
    edit->setStyleSheet({});
    ChangeColor(m_labelSecondRoleName, OkColor(this));
    CheckState();
}
