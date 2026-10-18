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
#include <QFrame>
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
    // Role name field A→C
    auto *containerRoleName = new QWidget();
    auto *layoutRoleName = new QHBoxLayout(containerRoleName);
    layoutRoleName->setContentsMargins(0, 0, 0, 0);
    layoutRoleName->setSpacing(4);
    auto *labelRoleNameDot = new QLabel();
    labelRoleNameDot->setObjectName(QStringLiteral("labelRoleNameDot"));
    labelRoleNameDot->setFixedSize(12, 12);
    m_labelRoleName = new QLabel(tr("Line:"));
    m_labelRoleName->setMinimumWidth(80);
    layoutRoleName->addWidget(labelRoleNameDot);
    layoutRoleName->addWidget(m_labelRoleName);
    layoutRoleName->addStretch();
    auto *lineEditRoleName = new QLineEdit();
    lineEditRoleName->setObjectName(QStringLiteral("lineEditRoleName"));
    auto *roleNameLayout = new QHBoxLayout;
    roleNameLayout->setContentsMargins(0, 0, 0, 0);
    roleNameLayout->addWidget(containerRoleName);
    roleNameLayout->addWidget(lineEditRoleName);
    ui->verticalLayout->insertLayout(0, roleNameLayout);
    connect(lineEditRoleName, &QLineEdit::textEdited, this,
            [this]()
            {
                ValidateLineRoleName();
                ValidateSecondLineRoleName();
            });

    // Role name field C→B (just below A→C)
    auto *containerSecondRoleName = new QWidget();
    auto *layoutSecondRoleName = new QHBoxLayout(containerSecondRoleName);
    layoutSecondRoleName->setContentsMargins(0, 0, 0, 0);
    layoutSecondRoleName->setSpacing(4);
    auto *labelSecondRoleNameDot = new QLabel();
    labelSecondRoleNameDot->setObjectName(QStringLiteral("labelSecondRoleNameDot"));
    labelSecondRoleNameDot->setFixedSize(12, 12);
    m_labelSecondRoleName = new QLabel(tr("Line:"));
    m_labelSecondRoleName->setMinimumWidth(80);
    layoutSecondRoleName->addWidget(labelSecondRoleNameDot);
    layoutSecondRoleName->addWidget(m_labelSecondRoleName);
    layoutSecondRoleName->addStretch();
    auto *lineEditSecondRoleName = new QLineEdit();
    lineEditSecondRoleName->setObjectName(QStringLiteral("lineEditSecondRoleName"));
    auto *secondRoleNameLayout = new QHBoxLayout;
    secondRoleNameLayout->setContentsMargins(0, 0, 0, 0);
    secondRoleNameLayout->addWidget(containerSecondRoleName);
    secondRoleNameLayout->addWidget(lineEditSecondRoleName);
    ui->verticalLayout->insertLayout(1, secondRoleNameLayout);
    connect(lineEditSecondRoleName, &QLineEdit::textEdited, this,
            [this]()
            {
                ValidateLineRoleName();
                ValidateSecondLineRoleName();
            });

    // Separator between the line name fields and Length
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout->insertLayout(2, lineSeparatorLayout);

    vis = new VisToolAlongLine(&this->data);

    // Call after initialization vis!!!!
    SetTypeLine(TypeLineNone); // By default don't show line

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::IsValid() const -> bool
{
    return m_flagName && m_formula->isValid() && flagRoleName && flagSecondRoleName;
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
                m_firstPointId = id;
                emit ToolTip(toolTip);
                m_number++;
                line->VisualMode(id);
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

    const QString formulaText = VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
        m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator());

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
    locData->SetTransientVariable(length);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::ChosenSecondPoint(quint32 id, const QString &toolTip)
{
    m_secondPointId = id;

    auto *line = qobject_cast<VisToolAlongLine *>(vis);
    SCASSERT(line != nullptr)

    if (m_firstPointId != m_secondPointId)
    {
        line->SetPoint2Id(id);
        // The value the tool lends has to exist as soon as the two points are known: a dialog that has not
        // been linked to a created tool yet has no `CurrentLength` in its container, and `LongueurActuelle`
        // is refused there. Midpoint only got it because it asked for it before prefilling its formula.
        SetCurrentLength();
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
    m_secondPointId = value;

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
void DialogAlongLine::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetFirstPointId(quint32 value)
{
    m_firstPointId = value;

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
        line->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
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
    return m_firstPointId;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAlongLine::GetSecondPointId() const -> quint32
{
    return m_secondPointId;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAlongLine::SetLineRoleName(const QString &suffix)
{
    m_initialRoleName = suffix;
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

    // A name designates one line: the pattern refuses a suffix another pair already holds, and the two lines of
    // this tool share the table. The variable map is no witness here — its keys go through VarToUser (`Line_`
    // becomes `Ligne_` in French), so a check written against it passes in every language but English. The
    // line's own name is not a clash with itself.
    const QString secondSuffix = GetSecondLineRoleName();
    const bool taken = data.IsLineRoleNameTaken(suffix) || not data.IsUnique(line_ + suffix)
                       || (not secondSuffix.isEmpty() && suffix == secondSuffix);
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
void DialogAlongLine::SetSecondLineRoleName(const QString &suffix)
{
    m_initialSecondRoleName = suffix;
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

    // Same rule, and the other line of this tool counts as another line: two rows under one name would leave
    // the second silently dropped by the pattern.
    const QString firstSuffix = GetLineRoleName();
    const bool taken = data.IsLineRoleNameTaken(suffix) || not data.IsUnique(line_ + suffix)
                       || (not firstSuffix.isEmpty() && suffix == firstSuffix);
    if (suffix != m_initialSecondRoleName && taken)
    {
        flagSecondRoleName = false;
        edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(m_labelSecondRoleName, errorColor);
        CheckState();
        return;
    }
    flagSecondRoleName = true;
    edit->setStyleSheet({});
    ChangeColor(m_labelSecondRoleName, OkColor(this));
    CheckState();
}
