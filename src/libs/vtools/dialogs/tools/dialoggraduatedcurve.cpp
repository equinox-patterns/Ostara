/************************************************************************
 **
 **  @file   dialoggraduatedcurve.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   4 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
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
#include "dialoggraduatedcurve.h"
#include "../../tools/toolsdef.h"
#include "../../visualization/path/vistoolgraduatedcurve.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../support/dialogeditwrongformula.h"
#include "../vgeometry/vsplinepath.h"
#include "vtheme.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "ui_dialoggraduatedcurve.h"
#include "vpatterndb/vformula.h"

#include <QFrame>
#include <QHBoxLayout>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

constexpr int DIALOG_MAX_FORMULA_HEIGHT = 64;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto AddCell(QTableWidget *table, const QString &text, int row, int column, int aligment, bool ok = true)
    -> QTableWidgetItem *
{
    SCASSERT(table != nullptr)

    auto *item = new QTableWidgetItem(text);
    SetTextAlignment(item, static_cast<Qt::Alignment>(aligment));
    item->setToolTip(text);

    // set the item non-editable (view only), and non-selectable
    Qt::ItemFlags flags = item->flags();
    flags &= ~(Qt::ItemIsEditable); // reset/clear the flag
    item->setFlags(flags);

    if (not ok)
    {
        QBrush brush = item->foreground();
        brush.setColor(Qt::red);
        item->setForeground(brush);
    }

    table->setItem(row, column, item);

    return item;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogGraduatedCurve::DialogGraduatedCurve(const VContainer *data,
                                           VAbstractPattern *doc,
                                           quint32 toolId,
                                           QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogGraduatedCurve)
{
    ui->setupUi(this);

    if (this->toolId == 0)
    { // fake id
        this->toolId = std::numeric_limits<quint32>::max();
    }

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxCurve);

    FillComboBoxCurves(ui->comboBoxCurve);

    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditCurveName, &QLineEdit::textEdited, this, &DialogGraduatedCurve::ValidateName);
    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogGraduatedCurve::ValidateAlias);

    // Move the curve-name field (Alias) to the top of the Tool tab (above the offsets)
    ui->labelAlias->setMinimumWidth(90);

    auto aliasRow = ui->formLayout_2->takeRow(ui->labelAlias);

    QWidget *aliasLabel = aliasRow.labelItem->widget();
    QWidget *aliasField = aliasRow.fieldItem->widget();
    delete aliasRow.labelItem;
    delete aliasRow.fieldItem;

    auto *aliasLayout = new QHBoxLayout;
    aliasLayout->setContentsMargins(0, 0, 0, 0);
    aliasLayout->addWidget(aliasLabel);
    aliasLayout->addWidget(aliasField);
    ui->verticalLayout_2->insertLayout(0, aliasLayout);

    // Separator between the curve-name field and the offsets
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout_2->insertLayout(1, lineSeparatorLayout);

    // Start/End point creation controls.
    m_startShowLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutStartPoint);
    connect(m_startShowLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { m_showLabelStart = checked; });

    m_endShowLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutEndPoint);
    connect(m_endShowLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { m_showLabelEnd = checked; });

    connect(ui->checkBoxCreateStartPoint, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && ui->lineEditStartPointName->text().isEmpty())
        {
            ui->lineEditStartPointName->setText(
                VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
        }
        UpdatePointControls();
    });

    connect(ui->checkBoxCreateEndPoint, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && ui->lineEditEndPointName->text().isEmpty())
        {
            ui->lineEditEndPointName->setText(
                VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
        }
        UpdatePointControls();
    });

    UpdatePointControls();

    vis = new VisToolGraduatedCurve(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);

    m_formulaBaseHeight = ui->plainTextEditFormula->height();
    ui->plainTextEditFormula->installEventFilter(this);

    ShowHeaderUnits(1); // calculated value
    ShowHeaderUnits(2); // formula

    FillOffsets();

    auto *validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^$|") + NameRegExp()), this);
    ui->lineEditName->setValidator(validator);

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &DialogGraduatedCurve::ShowOffsetDetails);

    connect(ui->toolButtonAdd, &QToolButton::clicked, this, &DialogGraduatedCurve::AddOffset);
    connect(ui->toolButtonRemove, &QToolButton::clicked, this, &DialogGraduatedCurve::RemoveOffset);
    connect(ui->toolButtonUp, &QToolButton::clicked, this, &DialogGraduatedCurve::MoveUp);
    connect(ui->toolButtonDown, &QToolButton::clicked, this, &DialogGraduatedCurve::MoveDown);
    connect(ui->pushButtonGrow, &QPushButton::clicked, this, &DialogGraduatedCurve::DeployFormula);
    connect(ui->toolButtonExpr, &QToolButton::clicked, this, &DialogGraduatedCurve::Fx);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DialogGraduatedCurve::ValidateOffsetName);
    connect(ui->lineEditName, &QLineEdit::editingFinished, this, &DialogGraduatedCurve::SaveOffsetName);
    connect(ui->plainTextEditDescription,
            &QPlainTextEdit::textChanged,
            this,
            &DialogGraduatedCurve::SaveOffsetDescription);
    connect(ui->plainTextEditFormula, &QPlainTextEdit::textChanged, this, &DialogGraduatedCurve::SaveOffsetFormula);

    if (ui->tableWidget->rowCount() > 0)
    {
        ui->tableWidget->selectRow(0);
    }

    InitOkCancelApply(ui);
}

//---------------------------------------------------------------------------------------------------------------------
DialogGraduatedCurve::~DialogGraduatedCurve()
{
    for (const auto &offset : std::as_const(m_offsets))
    {
        data.RemoveUniqueName(offset.name);
    }    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetOriginCurveId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCurve);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetOriginCurveId(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxCurve, value);

    // The preview can be gone — a full parse takes the scene and everything the dialog put on it — and the curve it
    // shows is set again as soon as it exists.
    if (auto *path = qobject_cast<VisToolGraduatedCurve *>(vis))
    {
        path->SetCurveId(value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetOffsets() const -> QVector<VRawGraduatedCurveOffset>
{
    QVector<VRawGraduatedCurveOffset> rawOffsets;
    rawOffsets.reserve(m_offsets.size());

    for (const auto &formulaData : m_offsets)
    {
        rawOffsets.append({.name = formulaData.name,
                           .formula = formulaData.formula.GetFormula(FormulaType::ToSystem),
                           .description = formulaData.description});
    }

    return rawOffsets;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetOffsets(const QVector<VRawGraduatedCurveOffset> &offsets)
{
    m_offsets.clear();
    m_offsets.reserve(offsets.size());

    auto localData = QSharedPointer<VContainer>(new VContainer(data));

    for (const auto &offset : offsets)
    {
        VFormula widthFormula(offset.formula, localData.data());
        widthFormula.setToolId(toolId);
        widthFormula.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
        widthFormula.Eval();

        m_offsets.append(
            {.name = offset.name, .formula = widthFormula, .formulaData = localData, .description = offset.description});

        auto newData = QSharedPointer<VContainer>(new VContainer(*localData.data()));

        auto *offsetVal = new VIncrement(newData.data(), offset.name);
        offsetVal->SetFormula(widthFormula.getDoubleValue(), offset.formula, widthFormula.error());
        offsetVal->SetDescription(
            offset.description.isEmpty()
                ? tr("Offset %1").arg(VAbstractApplication::VApp()->TrVars()->InternalVarToUser(offset.name))
                : offset.description);

        newData->AddUniqueVariable(offsetVal);

        localData = QSharedPointer<VContainer>(new VContainer(*newData.data()));
    }

    const int currentRow = ui->tableWidget->currentRow();
    FillOffsets();
    ui->tableWidget->selectRow(currentRow);

    auto *path = qobject_cast<VisToolGraduatedCurve *>(vis);
    if (path == nullptr)
    {
        // A full parse takes the canvas and this dialog's preview with it. There is nothing left to feed.
        return;
    }

    path->SetOffsets(VisualizationOffsets());
    // The preview follows the table here, and not only when the mouse happens to move over the canvas: the dialog is
    // modal when the widths are edited, so that move never comes. The row is read after the fill — the table is
    // filled with its signals blocked, so nothing else would tell the canvas which width the table is on.
    SetActiveOffset(ui->tableWidget->currentRow());
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetName(const QString &name)
{
    m_originName = name;
    ui->lineEditCurveName->setText(m_originName);
    ValidateName();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetName() const -> QString
{
    return ui->lineEditCurveName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::IsCreateStartPoint() const -> bool
{
    return ui->checkBoxCreateStartPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::IsCreateEndPoint() const -> bool
{
    return ui->checkBoxCreateEndPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetStartPointName() const -> QString
{
    return ui->lineEditStartPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetEndPointName() const -> QString
{
    return ui->lineEditEndPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetStartShowLabel() const -> bool
{
    return m_showLabelStart;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetEndShowLabel() const -> bool
{
    return m_showLabelEnd;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetStartPoint(bool exists, const QString &name, bool showLabel)
{
    m_startPointExists = exists;

    {
        const QSignalBlocker blocker(ui->checkBoxCreateStartPoint);
        ui->checkBoxCreateStartPoint->setChecked(exists);
    }

    if (exists)
    {
        const QSignalBlocker blocker(ui->lineEditStartPointName);
        ui->lineEditStartPointName->setText(name);

        m_showLabelStart = showLabel;
        if (m_startShowLabelCheckbox != nullptr)
        {
            const QSignalBlocker labelBlocker(m_startShowLabelCheckbox);
            m_startShowLabelCheckbox->setChecked(showLabel);
        }
    }

    UpdatePointControls();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SetEndPoint(bool exists, const QString &name, bool showLabel)
{
    m_endPointExists = exists;

    {
        const QSignalBlocker blocker(ui->checkBoxCreateEndPoint);
        ui->checkBoxCreateEndPoint->setChecked(exists);
    }

    if (exists)
    {
        const QSignalBlocker blocker(ui->lineEditEndPointName);
        ui->lineEditEndPointName->setText(name);

        m_showLabelEnd = showLabel;
        if (m_endShowLabelCheckbox != nullptr)
        {
            const QSignalBlocker labelBlocker(m_endShowLabelCheckbox);
            m_endShowLabelCheckbox->setChecked(showLabel);
        }
    }

    UpdatePointControls();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::UpdatePointControls()
{
    const bool startOn = ui->checkBoxCreateStartPoint->isChecked();
    const bool endOn = ui->checkBoxCreateEndPoint->isChecked();

    // A point already in the pattern is not renamed from here: its name is used by every formula that speaks about it,
    // and the property browser does not offer that either — the point itself does. The tab shows the name, read-only,
    // rather than a field that would take a new one and drop it.
    const QString startTip = m_startPointExists ? tr("The point exists already: rename it on the point itself.")
                                                : QString();
    const QString endTip = m_endPointExists ? tr("The point exists already: rename it on the point itself.")
                                            : QString();

    ui->lineEditStartPointName->setEnabled(startOn && not m_startPointExists);
    ui->lineEditStartPointName->setToolTip(startTip);
    ui->lineEditEndPointName->setEnabled(endOn && not m_endPointExists);
    ui->lineEditEndPointName->setToolTip(endTip);

    if (m_startShowLabelCheckbox != nullptr)
    {
        m_startShowLabelCheckbox->setEnabled(startOn);
    }
    if (m_endShowLabelCheckbox != nullptr)
    {
        m_endShowLabelCheckbox->setEnabled(endOn);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ShowDialog(bool click)
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

        const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
        QPointF const p = curve->ClosestPoint(scene->getScenePos());

        auto const line = QLineF(p, scene->getScenePos());
        if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
        {
            return;
        }

        qreal len = line.length();
        if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
        {
            len = -len;
        }

        QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

        rawOffsets.append(
            {.name = GetOffsetName(false), .formula = QString::number(FromPixel(len, *data.GetPatternUnit()))});

        SetOffsets(rawOffsets);
    }
    else
    {
        FinishCreating();
        if (!m_offsets.isEmpty())
        {
            ui->tableWidget->selectRow(0);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditCurveName->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if ((type == SceneObject::Arc || type == SceneObject::ElArc || type == SceneObject::Spline
         || type == SceneObject::SplinePath)
        && SetObject(id, ui->comboBoxCurve, QString()))
    {
        if (vis != nullptr)
        {
            vis->VisualMode(id);
        }
        prepare = true;

        SetName(GenerateDefOffsetCurveName(&data, GetOriginCurveId(), CopySuffix::OffsetCurve, "Curve"_L1 + offset_));

        auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
        SCASSERT(window != nullptr)
        connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, so that the options that follow have a preview to
        // talk to.
        vis = new VisToolGraduatedCurve(&this->data);
    }

    if (prepare)
    {
        // The curve is still being drawn: the preview belongs to that work, on the base curve the dialog has chosen.
        if (auto *path = qobject_cast<VisToolGraduatedCurve *>(vis); path != nullptr && GetOriginCurveId() != NULL_ID)
        {
            path->VisualMode(GetOriginCurveId());
        }
        return;
    }

    AddVisualization<VisToolGraduatedCurve>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SaveData()
{
    auto *path = qobject_cast<VisToolGraduatedCurve *>(vis);
    if (path == nullptr)
    {
        // A full parse takes the canvas and this dialog's preview with it. There is nothing left to feed.
        return;
    }

    path->SetCurveId(GetOriginCurveId());
    path->SetOffsets(GetOffsets());
    SetActiveOffset(ui->tableWidget->currentRow());
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::closeEvent(QCloseEvent *event)
{
    ui->plainTextEditFormula->blockSignals(true);
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons();
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ValidateName()
{
    const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
    const QString namePrefix = GetName();
    VSplinePath const splPath = curve->Outline({0}, namePrefix);

    if (QRegularExpression const rx(NameRegExp()); namePrefix.isEmpty() || not rx.match(splPath.name()).hasMatch()
                                                   || (m_originName != namePrefix && not data.IsUnique(splPath.name())))
    {
        m_flagName = false;
        // The field that carries the name being judged is the one of the curve: the offset's own name below has
        // nothing to do with it, and a red border there made a perfectly good name of an offset look refused.
        ui->lineEditCurveName->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelName, errorColor);
    }
    else
    {
        m_flagName = true;
        ui->lineEditCurveName->setStyleSheet({});
        ChangeColor(ui->labelName, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ValidateAlias()
{
    const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
    VSplinePath splPath = curve->Offset(0, GetName());

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
void DialogGraduatedCurve::ShowOffsetDetails()
{
    const int row = ui->tableWidget->currentRow();

    if (ui->tableWidget->rowCount() > 0 && row != -1)
    {
        EnableDetails(true);

        // The line being edited is the one width the canvas marks in the orange of the active point.
        SetActiveOffset(row);

        const VGraduatedCurveOffsetFormula &offsetData = m_offsets.at(row);

        {
            const QSignalBlocker blocker(ui->lineEditName);
            ui->lineEditName->setText(VAbstractApplication::VApp()->TrVars()->InternalVarToUser(offsetData.name));
            // A name refused on the line before must not leave its red border on this one.
            ui->lineEditName->setStyleSheet({});
            ui->lineEditName->setToolTip({});
        }

        if (QString const unitSuffix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());
            offsetData.formula.error())
        {
            ui->labelCalculatedValue->setText(tr("Error") + " ("_L1 + unitSuffix + "). "_L1);
            ui->labelCalculatedValue->setToolTip(offsetData.formula.Reason());
        }
        else
        {
            ui->labelCalculatedValue->setText(offsetData.formula.getStringValue());
            ui->labelCalculatedValue->setToolTip(tr("Value"));
        }

        {
            const QSignalBlocker blocker(ui->plainTextEditFormula);
            QString const formula = offsetData.formula.GetFormula(FormulaType::ToUser);
            ui->plainTextEditFormula->setPlainText(formula);
        }

        const QSignalBlocker blocker(ui->plainTextEditDescription);
        ui->plainTextEditDescription->setPlainText(offsetData.description);

        return;
    }

    SetActiveOffset(-1);
    EnableDetails(false);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Tells the preview which width the table is on, so that the canvas marks where that one width acts. A table
 *  without a line leaves every mark of the preview in its own colour. */
void DialogGraduatedCurve::SetActiveOffset(int row)
{
    auto *path = qobject_cast<VisToolGraduatedCurve *>(vis);
    if (path != nullptr)
    {
        path->SetActiveOffset(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::AddOffset()
{
    const QString name = GetOffsetName(false);
    qint32 currentRow = -1;

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    if (ui->tableWidget->currentRow() == -1)
    {
        currentRow = ui->tableWidget->rowCount();

        rawOffsets.append({.name = name, .formula = "0"_L1});
    }
    else
    {
        currentRow = ui->tableWidget->currentRow() + 1;

        if (currentRow >= 0 && currentRow <= rawOffsets.size())
        {
            rawOffsets.insert(currentRow, {.name = name, .formula = "0"_L1});
        }
    }

    SetOffsets(rawOffsets);

    ui->tableWidget->selectRow(currentRow);
    ui->tableWidget->repaint(); // Force repain to fix paint artifacts on Mac OS
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::RemoveOffset()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1)
    {
        return;
    }

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    if (row >= 0 && row < rawOffsets.size())
    {
        data.RemoveUniqueName(rawOffsets.at(row).name);
        rawOffsets.removeAt(row);
    }

    SetOffsets(rawOffsets);

    ui->tableWidget->rowCount() > 0 ? ui->tableWidget->selectRow(0) : EnableDetails(false);
    ui->tableWidget->repaint(); // Force repain to fix paint artifacts on Mac OS X

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::MoveUp()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1 || row == 0)
    {
        return;
    }

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    if (row > 0 && row < rawOffsets.size())
    {
        rawOffsets.move(row, row - 1);
    }

    SetOffsets(rawOffsets);

    ui->tableWidget->selectRow(row - 1);
    ui->tableWidget->repaint(); // Force repain to fix paint artifacts on Mac OS X
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::MoveDown()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1 || row == ui->tableWidget->rowCount() - 1)
    {
        return;
    }

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    if (row >= 0 && row < rawOffsets.size() - 1)
    {
        rawOffsets.move(row, row + 1);
    }

    SetOffsets(rawOffsets);

    ui->tableWidget->selectRow(row + 1);
    ui->tableWidget->repaint(); // Force repain to fix paint artifacts on Mac OS X
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::DeployFormula()
{
    const QTextCursor cursor = ui->plainTextEditFormula->textCursor();

    if (ui->plainTextEditFormula->height() < DIALOG_MAX_FORMULA_HEIGHT)
    {
        ui->plainTextEditFormula->setFixedHeight(DIALOG_MAX_FORMULA_HEIGHT);
        // Set icon from theme (internal for Windows system)
        ui->pushButtonGrow->setIcon(FromTheme(VThemeIcon::GoNext));
    }
    else
    {
        ui->plainTextEditFormula->setFixedHeight(m_formulaBaseHeight);
        // Set icon from theme (internal for Windows system)
        ui->pushButtonGrow->setIcon(FromTheme(VThemeIcon::GoDown));
    }

    // I found that after change size of formula field, it was filed for angle formula, field for formula became black.
    // This code prevents this.
    setUpdatesEnabled(false);
    repaint();
    setUpdatesEnabled(true);

    ui->plainTextEditFormula->setFocus();
    ui->plainTextEditFormula->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::Fx()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1)
    {
        return;
    }

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    if (row < 0 || row >= rawOffsets.size() || row >= m_offsets.size())
    {
        return;
    }

    VRawGraduatedCurveOffset &offsetData = rawOffsets[row];

    QScopedPointer<DialogEditWrongFormula> const dialog(
        new DialogEditWrongFormula(m_offsets.at(row).formulaData.data(), toolId, this));
    dialog->setWindowTitle(tr("Edit offset"));
    dialog->SetFormula(offsetData.formula);
    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    dialog->setPostfix(postfix); // Show unit in dialog lable (cm, mm or inch)

    if (dialog->exec() == QDialog::Accepted)
    {
        offsetData.formula = dialog->GetFormula();
        SetOffsets(rawOffsets);

        ui->tableWidget->selectRow(row);
        ShowOffsetDetails();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The reason a name cannot be given to the line @p row, or an empty string when it can. The name of an offset
 *  is a variable of the pattern: empty, ill-formed or already taken names are not names. */
auto DialogGraduatedCurve::OffsetNameProblem(int row, const QString &name) const -> QString
{
    if (name.isEmpty())
    {
        return tr("An offset must carry a name.");
    }

    if (not QRegularExpression(NameRegExp()).match(name).hasMatch())
    {
        return tr("This name cannot be used as the name of a variable.");
    }

    if (row >= 0 && row < m_offsets.size() && name == m_offsets.at(row).name)
    {
        return {}; // the name it carries already: nothing to change, and nothing to complain about
    }

    const VContainer *localData = m_offsets.isEmpty() ? &data : m_offsets.constLast().formulaData.data();
    if (not localData->IsUnique(name))
    {
        return tr("Another variable of the pattern already carries this name.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief ValidateOffsetName says, while the name is typed, whether the line being edited can take it. The line is
 *  remembered here: by the time the field is left, the table may well be on another one. */
void DialogGraduatedCurve::ValidateOffsetName()
{
    const int row = ui->tableWidget->currentRow();
    if (row == -1)
    {
        return;
    }

    m_namedOffsetRow = row;
    m_namedOffsetText = ui->lineEditName->text();

    const QString name = VAbstractApplication::VApp()->TrVars()->InternalVarFromUser(ui->lineEditName->text());
    const QString problem = OffsetNameProblem(row, name);

    ui->lineEditName->setStyleSheet(problem.isEmpty() ? QString() : QStringLiteral("border: 1px solid red;"));
    ui->lineEditName->setToolTip(problem);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief SaveOffsetName gives the line being edited the name typed in the field, once the field is left.
 *
 *  Not on every keystroke: a half-typed name — "Distance_" while the number after it is being changed — is a valid
 *  name all by itself, and renaming on the way left the offset named after something nobody ever typed. */
void DialogGraduatedCurve::SaveOffsetName()
{
    const int row = m_namedOffsetRow;
    const QString typed = m_namedOffsetText;
    m_namedOffsetRow = -1;
    m_namedOffsetText.clear();

    // Leaving the field after the table has moved to another line: the name typed speaks for a line the field no
    // longer shows, and there is nothing to rename.
    if (row == -1 || row >= m_offsets.size() || ui->lineEditName->text() != typed)
    {
        return;
    }

    const QString newName = VAbstractApplication::VApp()->TrVars()->InternalVarFromUser(ui->lineEditName->text());
    const QString problem = OffsetNameProblem(row, newName);

    if (not problem.isEmpty())
    {
        // The field carries a name the pattern cannot take: say why, and put back the one the offset really has, so
        // that the line and the field never tell two different stories.
        ui->lineEditName->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ui->lineEditName->setToolTip(problem);

        const QSignalBlocker blocker(ui->lineEditName);
        ui->lineEditName->setText(VAbstractApplication::VApp()->TrVars()->InternalVarToUser(m_offsets.at(row).name));
        return;
    }

    ui->lineEditName->setStyleSheet({});
    ui->lineEditName->setToolTip({});

    if (newName == m_offsets.at(row).name)
    {
        return;
    }

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();

    const QString oldName = rawOffsets.at(row).name;

    // The name of an offset is a variable name the pattern reserves: the old one has to be given back, or the number
    // it held stays taken for good, and coming back to it later would be refused for no visible reason.
    data.RemoveUniqueName(oldName);

    // Whole names only: the formula of the next offset may well hold "Offset_10", which renaming "Offset_1" must not
    // touch (the chain follows the name, not the text that happens to spell it).
    const QRegularExpression oldNamePattern(QStringLiteral("\\b") + QRegularExpression::escape(oldName)
                                            + QStringLiteral("\\b"));

    for (int i = row + 1; i < rawOffsets.size(); ++i)
    {
        if (VRawGraduatedCurveOffset &offset = rawOffsets[i]; offset.formula.contains(oldNamePattern))
        {
            offset.formula.replace(oldNamePattern, newName);
        }
    }

    rawOffsets[row].name = newName;

    SetOffsets(rawOffsets);

    {
        const QSignalBlocker blocker(ui->tableWidget);
        ui->tableWidget->selectRow(row);
    }

    // The field shows the name the pattern took, in the user's own words: what was typed may have been another
    // spelling of it.
    const QSignalBlocker blocker(ui->lineEditName);
    ui->lineEditName->setText(VAbstractApplication::VApp()->TrVars()->InternalVarToUser(newName));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SaveOffsetDescription()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1 || row >= m_offsets.size())
    {
        return;
    }

    const QTextCursor cursor = ui->plainTextEditDescription->textCursor();

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();
    rawOffsets[row].description = ui->plainTextEditDescription->toPlainText();
    SetOffsets(rawOffsets);

    {
        const QSignalBlocker blocker(ui->tableWidget);
        ui->tableWidget->selectRow(row);
    }
    ui->plainTextEditDescription->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::SaveOffsetFormula()
{
    const int row = ui->tableWidget->currentRow();

    if (row == -1 || row >= m_offsets.size())
    {
        return;
    }

    QString const text = ui->plainTextEditFormula->toPlainText();
    QString const units = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());

    VFormula widthFormula(m_offsets.at(row).formulaData.data());
    widthFormula.SetFormula(text, FormulaType::FromUser);
    widthFormula.setToolId(toolId);
    widthFormula.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    widthFormula.Eval();

    if (widthFormula.error())
    {
        ui->labelCalculatedValue->setText(tr("Error") + " ("_L1 + units + "). "_L1 + widthFormula.Reason());
        ui->labelCalculatedValue->setToolTip(tr("Parser error: %1").arg(widthFormula.Reason()));
        return;
    }

    ui->labelCalculatedValue->setText(widthFormula.getStringValue());
    ui->labelCalculatedValue->setToolTip(tr("Value"));

    QVector<VRawGraduatedCurveOffset> rawOffsets = GetOffsets();
    rawOffsets[row].formula = widthFormula.GetFormula(FormulaType::ToSystem);

    const QTextCursor cursor = ui->plainTextEditFormula->textCursor();

    SetOffsets(rawOffsets);

    {
        const QSignalBlocker blocker(ui->tableWidget);
        ui->tableWidget->selectRow(row);
    }
    ui->plainTextEditFormula->setTextCursor(cursor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::FinishCreating()
{
    if (vis != nullptr)
    {
        vis->SetMode(Mode::Show);
        vis->RefreshGeometry();
    }

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::InitIcons()
{
    const auto resource = QStringLiteral("icon");

    const auto fxIcon = QStringLiteral("24x24/fx.png");
    ui->toolButtonExpr->setIcon(VTheme::GetIconResource(resource, fxIcon));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::FillOffsets()
{
    const QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(static_cast<int>(m_offsets.size()));

    for (int i = 0; i < m_offsets.size(); ++i)
    {
        const VGraduatedCurveOffsetFormula &offsetData = m_offsets.at(i);
        // The name of an offset is written in the pattern's own words — "Offset_1" — but the whole dialog shows it
        // the way the user reads it, here and in the Name field below, like the formulas of the next column.
        AddCell(ui->tableWidget, VAbstractApplication::VApp()->TrVars()->InternalVarToUser(offsetData.name), i, 0,
                Qt::AlignVCenter); // name

        VFormula const formula = offsetData.formula;
        QString const calculatedValue = VAbstractApplication::VApp()->LocaleToString(formula.getDoubleValue());
        AddCell(ui->tableWidget, calculatedValue, i, 1, Qt::AlignCenter, !formula.error()); // calculated value

        QString const formulaStr = formula.GetFormula(FormulaType::ToUser);
        AddCell(ui->tableWidget, formulaStr, i, 2, Qt::AlignVCenter); // formula
    }

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::Controls()
{
    ui->toolButtonRemove->setEnabled(ui->tableWidget->rowCount() > 0);

    if (ui->tableWidget->rowCount() >= 2)
    {
        if (ui->tableWidget->currentRow() == 0)
        {
            ui->toolButtonUp->setEnabled(false);
            ui->toolButtonDown->setEnabled(true);
        }
        else if (ui->tableWidget->currentRow() == ui->tableWidget->rowCount() - 1)
        {
            ui->toolButtonUp->setEnabled(true);
            ui->toolButtonDown->setEnabled(false);
        }
        else
        {
            ui->toolButtonUp->setEnabled(true);
            ui->toolButtonDown->setEnabled(true);
        }
    }
    else
    {
        ui->toolButtonUp->setEnabled(false);
        ui->toolButtonDown->setEnabled(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::EnableDetails(bool enabled)
{
    if (enabled)
    {
        Controls();
    }
    else
    {
        ui->toolButtonRemove->setEnabled(enabled);
        ui->toolButtonUp->setEnabled(enabled);
        ui->toolButtonDown->setEnabled(enabled);
    }

    if (not enabled)
    { // Clear
        {
            const QSignalBlocker blocker(ui->lineEditName);
            ui->lineEditName->clear();
        }

        {
            const QSignalBlocker blocker(ui->plainTextEditDescription);
            ui->plainTextEditDescription->clear();
        }

        {
            const QSignalBlocker blocker(ui->labelCalculatedValue);
            ui->labelCalculatedValue->clear();
        }

        const QSignalBlocker blocker(ui->plainTextEditFormula);
        ui->plainTextEditFormula->clear();
    }

    ui->pushButtonGrow->setEnabled(enabled);
    ui->toolButtonExpr->setEnabled(enabled);
    ui->lineEditName->setEnabled(enabled);
    ui->plainTextEditDescription->setEnabled(enabled);
    ui->plainTextEditFormula->setEnabled(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::GetOffsetName(bool translate) const -> QString
{
    const VContainer *localData = m_offsets.isEmpty() ? &data : m_offsets.constLast().formulaData.data();
    qint32 num = 1;
    QString name;
    QString const subName = translate ? VAbstractApplication::VApp()->TrVars()->InternalVarToUser(offset_) : offset_;
    do
    {
        name = subName + QString::number(num);
        num++;
    } while (not localData->IsUnique(name));
    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::VisualizationOffsets() const -> QVector<VRawGraduatedCurveOffset>
{
    QVector<VRawGraduatedCurveOffset> toUserOffsets;
    toUserOffsets.reserve(m_offsets.size());
    for (const auto &offset : std::as_const(m_offsets))
    {
        toUserOffsets.append({.name = offset.name,
                              .formula = offset.formula.GetFormula(FormulaType::ToUser),
                              .description = offset.description});
    }
    return toUserOffsets;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGraduatedCurve::ShowHeaderUnits(int column)
{
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->patternUnits());
    const QString header = ui->tableWidget->horizontalHeaderItem(column)->text();
    const auto unitHeader = QStringLiteral("%1 (%2)").arg(header, unit);
    ui->tableWidget->horizontalHeaderItem(column)->setText(unitHeader);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGraduatedCurve::IsValid() const -> bool
{
    bool const flagOffset = std::any_of(m_offsets.constBegin(),
                                        m_offsets.constEnd(),
                                        [](const auto &offset) -> auto { return !offset.formula.error(); });

    return !m_offsets.isEmpty() && flagOffset && m_flagName && m_flagAlias;
}
