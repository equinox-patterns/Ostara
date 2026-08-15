/************************************************************************
 **
 **  @file   dialogsplinepath.cpp
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

#include "dialogsplinepath.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>
#include <QVariant>

#include "../../visualization/path/vistoolsplinepath.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../widgets/formulainputwidget.h"
#include "../vgeometry/../ifc/ifcdef.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vsplinepoint.h"
#include "vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmousewheelwidgetadjustmentguard.h"
#include "ui_dialogsplinepath.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
// Per-item handle validity bitmask, stored in QListWidgetItem under this role.
// A set bit means the corresponding control-handle field is valid.
constexpr int validityRole = Qt::UserRole + 1;
constexpr int flagLength1 = 0x1;
constexpr int flagAngle1 = 0x2;
constexpr int flagLength2 = 0x4;
constexpr int flagAngle2 = 0x8;
constexpr int allHandleFlags = flagLength1 | flagAngle1 | flagLength2 | flagAngle2;

void SetPointValidity(QListWidgetItem *item, int flag, bool valid)
{
    int validity = item->data(validityRole).toInt();
    if (valid)
    {
        validity |= flag;
    }
    else
    {
        validity &= ~flag;
    }
    item->setData(validityRole, validity);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogSplinePath create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogSplinePath::DialogSplinePath(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogSplinePath)
{
    ui->setupUi(this);

    // Prevent stealing focus when scrolling
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->comboBoxNewPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->comboBoxPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->comboBoxPenStyle);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->doubleSpinBoxApproximationScale);


    MakeFilterableComboBox(ui->comboBoxNewPoint);
    MakeFilterableComboBox(ui->comboBoxPoint);


    InitOkCancelApply(ui);
    bOk->setEnabled(false);

    FillComboBoxPoints(ui->comboBoxNewPoint);
    FillComboBoxPoints(ui->comboBoxPoint);

    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaLength1 = new FormulaInputWidget(this);
    m_formulaLength1->setData(&this->data);
    m_formulaLength1->setToolId(toolId);
    m_formulaLength1->setLabelText(tr("Length:"));
    m_formulaLength1->setPostfix(lengthPostfix);
    m_formulaLength1->setDialogTitle(tr("Edit first control point length"));
    m_formulaLength1->setCheckLessThanZero(true);

    connect(m_formulaLength1, &FormulaInputWidget::valueChanged, this, [this]() { Length1Changed(); });

    m_formulaAngle1 = new FormulaInputWidget(this);
    m_formulaAngle1->setData(&this->data);
    m_formulaAngle1->setToolId(toolId);
    m_formulaAngle1->setLabelText(tr("Angle:"));
    m_formulaAngle1->setPostfix(degreeSymbol);
    m_formulaAngle1->setDialogTitle(tr("Edit first control point angle"));
    connect(m_formulaAngle1, &FormulaInputWidget::valueChanged, this, [this]() { Angle1Changed(); });

    m_formulaLength2 = new FormulaInputWidget(this);
    m_formulaLength2->setData(&this->data);
    m_formulaLength2->setToolId(toolId);
    m_formulaLength2->setLabelText(tr("Length:"));
    m_formulaLength2->setPostfix(lengthPostfix);
    m_formulaLength2->setDialogTitle(tr("Edit second control point length"));
    m_formulaLength2->setCheckLessThanZero(true);
    connect(m_formulaLength2, &FormulaInputWidget::valueChanged, this, [this]() { Length2Changed(); });

    m_formulaAngle2 = new FormulaInputWidget(this);
    m_formulaAngle2->setData(&this->data);
    m_formulaAngle2->setToolId(toolId);
    m_formulaAngle2->setLabelText(tr("Angle:"));
    m_formulaAngle2->setPostfix(degreeSymbol);
    m_formulaAngle2->setDialogTitle(tr("Edit second control point angle"));
    connect(m_formulaAngle2, &FormulaInputWidget::valueChanged, this, [this]() { Angle2Changed(); });

    // Replace placeholders
    auto replacePlaceholder = [this](const char *name, FormulaInputWidget *widget)
    {
        auto *placeholder = findChild<QWidget *>(QString::fromLatin1(name));
        if (placeholder)
        {
            auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
            if (parentLayout != nullptr)
            {
                const int idx = parentLayout->indexOf(placeholder);
                parentLayout->insertWidget(idx, widget);
                placeholder->deleteLater();
            }
        }
    };

    replacePlaceholder("formulaLength1Placeholder", m_formulaLength1);
    replacePlaceholder("formulaAngle1Placeholder", m_formulaAngle1);
    replacePlaceholder("formulaLength2Placeholder", m_formulaLength2);
    replacePlaceholder("formulaAngle2Placeholder", m_formulaAngle2);

    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    ui->doubleSpinBoxApproximationScale->setMaximum(maxCurveApproximationScale);

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &DialogSplinePath::PointChanged);
    connect(ui->comboBoxPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSplinePath::currentPointChanged);
    connect(ui->comboBoxNewPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSplinePath::NewPointChanged);

    connect(ui->toolButtonAddPoint, &QToolButton::clicked, this, &DialogSplinePath::AddPoint);
    connect(ui->toolButtonRemovePoint, &QToolButton::clicked, this, &DialogSplinePath::RemovePoint);

    connect(ui->toolButtonTop, &QToolButton::clicked, this, &DialogSplinePath::MoveTop);
    connect(ui->toolButtonUp, &QToolButton::clicked, this, &DialogSplinePath::MoveUp);
    connect(ui->toolButtonDown, &QToolButton::clicked, this, &DialogSplinePath::MoveDown);
    connect(ui->toolButtonBottom, &QToolButton::clicked, this, &DialogSplinePath::MoveBottom);




    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogSplinePath::ValidateAlias);

    vis = new VisToolSplinePath(&this->data);
    auto *path = qobject_cast<VisToolSplinePath *>(vis);
    SCASSERT(path != nullptr)

    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    SCASSERT(scene != nullptr)
    connect(scene, &VMainGraphicsScene::MouseLeftPressed, path, &VisToolSplinePath::MouseLeftPressed);
    connect(scene, &VMainGraphicsScene::MouseLeftReleased, path, &VisToolSplinePath::MouseLeftReleased);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogSplinePath::~DialogSplinePath()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPath set spline path
 * @param value path
 */
void DialogSplinePath::SetPath(const VSplinePath &value)
{
    path = value;

    m_oldName = path.name();

    {
        const QSignalBlocker blocker(ui->listWidget);
        ui->listWidget->clear();
        for (qint32 i = 0; i < path.CountPoints(); ++i)
        {
            NewItem(path.at(i));
        }
        ui->listWidget->setFocus(Qt::OtherFocusReason);
        ui->doubleSpinBoxApproximationScale->setValue(path.GetApproximationScale());

        originAliasSuffix = path.GetAliasSuffix();
        ui->lineEditAlias->setText(originAliasSuffix);
        ValidateAlias();

        ChangeCurrentData(ui->comboBoxPenStyle, path.GetPenStyle());
        SetColorComboCurrentName(m_colorCombo, path.GetColor());

        auto *visPath = qobject_cast<VisToolSplinePath *>(vis);
        SCASSERT(visPath != nullptr)
        visPath->SetPath(path);
    }

    flagError = IsPathValid();

    ui->toolButtonRemovePoint->setEnabled(ui->listWidget->count() > 3);
    MoveControls();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type don't show this id in list
 */
void DialogSplinePath::ChosenObject(quint32 id, const SceneObject &type)
{
    if (type == SceneObject::Point)
    {
        if (AllIds().contains(id))
        {
            return;
        }

        const auto point = data.GeometricObject<VPointF>(id);
        VSplinePoint p;
        p.SetP(*point);
        NewItem(p);
        emit ToolTip(tr("Select point of curve path"));

        SavePath();

        auto *visPath = qobject_cast<VisToolSplinePath *>(vis);
        SCASSERT(visPath != nullptr)
        visPath->SetPath(path);

        if (path.CountPoints() == 1 && not m_visConnected)
        {
            m_visConnected = true;
            visPath->VisualMode(NULL_ID);
            auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(visPath, &VisToolSplinePath::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

            connect(visPath, &VisToolSplinePath::PathChanged, this, &DialogSplinePath::PathUpdated);
        }
        else
        {
            visPath->RefreshGeometry();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveData Put dialog data in local variables
 */
void DialogSplinePath::SaveData()
{
    SavePath();

    if (!m_oldName.isEmpty() && m_oldName != path.name())
    {
        path.SetDuplicate(0);
        if (!data.IsUnique(path.name()))
        {
            path.SetDuplicate(DNumber(path.name()));
            m_oldName = path.name();
        }
    }

    auto *visPath = qobject_cast<VisToolSplinePath *>(vis);
    SCASSERT(visPath != nullptr)
    visPath->SetPath(path);
    visPath->SetMode(Mode::Show);
    visPath->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::changeEvent(QEvent *event)
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
void DialogSplinePath::ValidateAlias()
{
    VSplinePath tempPath = path;
    tempPath.SetAliasSuffix(ui->lineEditAlias->text());

    if (QRegularExpression const rx(NameRegExp());
        not ui->lineEditAlias->text().isEmpty() &&
        (not rx.match(tempPath.GetAlias()).hasMatch() ||
         (originAliasSuffix != ui->lineEditAlias->text() && not data.IsUnique(tempPath.GetAlias()))))
    {
        flagAlias = false;
        ui->lineEditAlias->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias, errorColor);
    }
    else
    {
        flagAlias = true;
        ui->lineEditAlias->setStyleSheet({});
        ChangeColor(ui->labelAlias, OkColor(this));
    }

    CheckState();
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PointChanged selected another point in list
 * @param row number of row
 */
void DialogSplinePath::PointChanged(int row)
{
    if (ui->listWidget->count() == 0 || row < 0)
    {
        ui->toolButtonTop->setEnabled(false);
        ui->toolButtonUp->setEnabled(false);
        ui->toolButtonDown->setEnabled(false);
        ui->toolButtonBottom->setEnabled(false);
        return;
    }

    MoveControls();

    const auto p = qvariant_cast<VSplinePoint>(ui->listWidget->item(row)->data(Qt::UserRole));
    DataPoint(p);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief currentPointChanged changed point in combo box
 * @param index index in list
 */
void DialogSplinePath::currentPointChanged(int index)
{
    const auto id = qvariant_cast<quint32>(ui->comboBoxPoint->itemData(index));
    QColor color;

    if (ui->listWidget->currentRow() < 0)
    {
        return;
    }

    try
    {
        QListWidgetItem *item = ui->listWidget->item(ui->listWidget->currentRow());
        auto p = qvariant_cast<VSplinePoint>(item->data(Qt::UserRole));

        const auto point = data.GeometricObject<VPointF>(id);
        p.SetP(*point);

        DataPoint(p);
        item->setData(Qt::UserRole, QVariant::fromValue(p));
        ShowPointIssue(p.P().name());

        if (not IsPathValid())
        {
            flagError = false;
            color = errorColor;
        }
        else
        {
            flagError = true;
            color = OkColor(this);
        }
    }
    catch (const VExceptionBadId &)
    {
        flagError = false;
        color = errorColor;
    }

    ChangeColor(ui->labelPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::NewPointChanged()
{
    ui->toolButtonAddPoint->setEnabled(m_dependencyReady && ui->comboBoxNewPoint->currentIndex() != -1);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::AddPoint()
{
    const auto id = qvariant_cast<quint32>(ui->comboBoxNewPoint->currentData());
    const auto point = data.GeometricObject<VPointF>(id);
    VSplinePoint p;
    p.SetP(*point);
    NewItem(p);
    SavePath();

    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.

    {
        const QSignalBlocker blocker(ui->comboBoxNewPoint);
        ui->comboBoxNewPoint->setCurrentIndex(-1);
    }
    ui->toolButtonAddPoint->setDisabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::RemovePoint()
{
    if (ui->listWidget->count() <= 3)
    {
        ui->toolButtonRemovePoint->setDisabled(true);
        return;
    }

    QListWidgetItem  const*selectedItem = ui->listWidget->currentItem();
    if (selectedItem)
    {
        delete ui->listWidget->takeItem(ui->listWidget->row(selectedItem));
        ui->listWidget->setCurrentRow(0);
        ui->toolButtonRemovePoint->setDisabled(ui->listWidget->count() <= 3);
    }

    SavePath();

    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveTop()
{
    MoveListRowTop(ui->listWidget);
    SavePath();
    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveUp()
{
    MoveListRowUp(ui->listWidget);
    SavePath();
    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveDown()
{
    MoveListRowDown(ui->listWidget);
    SavePath();
    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveBottom()
{
    MoveListRowBottom(ui->listWidget);
    SavePath();
    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::ShowDialog(bool click)
{
    if (click)
    {
        return;
    }

    if (path.CountPoints() >= 3)
    {
        emit ToolTip(QString());

        if (not data.IsUnique(path.name()))
        {
            path.SetDuplicate(DNumber(path.name()));
        }

        DialogAccepted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::PathUpdated(const VSplinePath &path)
{
    SetPath(path);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::ShowVisualization()
{
    AddVisualization<VisToolSplinePath>();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NewItem add point to list
 * @param point spline path point
 */
void DialogSplinePath::NewItem(const VSplinePoint &point)
{
    auto *item = new QListWidgetItem(point.P().name());
    item->setFont(NodeFont(item->font()));
    item->setData(Qt::UserRole, QVariant::fromValue(point));
    item->setData(validityRole, allHandleFlags);

    ui->listWidget->addItem(item);
    ui->listWidget->setCurrentItem(item);
    if (ui->listWidget->count() >= 2)
    {
        bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
        bOk->setEnabled(true);
        ui->toolButtonRemovePoint->setEnabled(true);
    }

    DataPoint(point);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dataPoint show data of point in fields
 * @param p spline path point
 */
void DialogSplinePath::DataPoint(const VSplinePoint &p)
{
    {
        const QSignalBlocker blocker(ui->comboBoxPoint);
        ChangeCurrentData(ui->comboBoxPoint, p.P().id());
    }

    int const row = ui->listWidget->currentRow();
    const QString field = tr("Not used");

    if (row == 0)
    {
        m_formulaAngle1->setEnabled(false);
        m_formulaAngle1->setFormula(field);
        m_formulaLength1->setEnabled(false);
        m_formulaLength1->setFormula(field);

        m_formulaAngle2->setEnabled(true);
        m_formulaLength2->setEnabled(true);

        m_formulaAngle2->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Angle2Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        m_formulaLength2->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Length2Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else if (row == ui->listWidget->count() - 1)
    {
        m_formulaAngle2->setEnabled(false);
        m_formulaAngle2->setFormula(field);
        m_formulaLength2->setEnabled(false);
        m_formulaLength2->setFormula(field);

        m_formulaAngle1->setEnabled(true);
        m_formulaLength1->setEnabled(true);

        m_formulaAngle1->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Angle1Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        m_formulaLength1->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Length1Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        m_formulaAngle1->setEnabled(true);
        m_formulaLength1->setEnabled(true);
        m_formulaAngle2->setEnabled(true);
        m_formulaLength2->setEnabled(true);

        m_formulaAngle1->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Angle1Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        m_formulaAngle2->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Angle2Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        m_formulaLength1->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Length1Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        m_formulaLength2->setFormula(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            p.Length2Formula(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Angle1Changed()
{
    if (m_formulaAngle1 == nullptr || not m_formulaAngle1->isEnabled())
    {
        return;
    }

    const int row = ui->listWidget->currentRow();
    if (row < 0 || row >= ui->listWidget->count())
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->item(row);
    SCASSERT(item != nullptr)
    auto p = qvariant_cast<VSplinePoint>(item->data(Qt::UserRole));

    const bool valid = m_formulaAngle1->isValid();
    SetPointValidity(item, flagAngle1, valid);

    if (valid)
    {
        const QString f = m_formulaAngle1->formula();
        const qreal value = Visualization::FindValFromUser(f, data.DataVariables(), false);
        if (not VFuzzyComparePossibleNulls(value, p.Angle1()))
        {
            p.SetAngle1(value, f);
            item->setData(Qt::UserRole, QVariant::fromValue(p));

            // Strict coupling: the opposite angle follows. Reflect it in the UI.
            if (row != ui->listWidget->count() - 1)
            {
                m_formulaAngle2->setFormula(p.Angle2Formula());
                SetPointValidity(item, flagAngle2, true);
            }
        }
    }

    ShowPointIssue(p.P().name());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Angle2Changed()
{
    if (m_formulaAngle2 == nullptr || not m_formulaAngle2->isEnabled())
    {
        return;
    }

    const int row = ui->listWidget->currentRow();
    if (row < 0 || row >= ui->listWidget->count())
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->item(row);
    SCASSERT(item != nullptr)
    auto p = qvariant_cast<VSplinePoint>(item->data(Qt::UserRole));

    const bool valid = m_formulaAngle2->isValid();
    SetPointValidity(item, flagAngle2, valid);

    if (valid)
    {
        const QString f = m_formulaAngle2->formula();
        const qreal value = Visualization::FindValFromUser(f, data.DataVariables(), false);
        if (not VFuzzyComparePossibleNulls(value, p.Angle2()))
        {
            p.SetAngle2(value, f);
            item->setData(Qt::UserRole, QVariant::fromValue(p));

            if (row != 0)
            {
                m_formulaAngle1->setFormula(p.Angle1Formula());
                SetPointValidity(item, flagAngle1, true);
            }
        }
    }

    ShowPointIssue(p.P().name());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Length1Changed()
{
    if (m_formulaLength1 == nullptr || not m_formulaLength1->isEnabled())
    {
        return;
    }

    const int row = ui->listWidget->currentRow();
    if (row < 0 || row >= ui->listWidget->count())
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->item(row);
    SCASSERT(item != nullptr)
    auto p = qvariant_cast<VSplinePoint>(item->data(Qt::UserRole));

    const bool valid = m_formulaLength1->isValid();
    SetPointValidity(item, flagLength1, valid);

    if (valid)
    {
        const QString f = m_formulaLength1->formula();
        p.SetLength1(Visualization::FindLengthFromUser(f, data.DataVariables(), false), f);
        item->setData(Qt::UserRole, QVariant::fromValue(p));
    }

    ShowPointIssue(p.P().name());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Length2Changed()
{
    if (m_formulaLength2 == nullptr || not m_formulaLength2->isEnabled())
    {
        return;
    }

    const int row = ui->listWidget->currentRow();
    if (row < 0 || row >= ui->listWidget->count())
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->item(row);
    SCASSERT(item != nullptr)
    auto p = qvariant_cast<VSplinePoint>(item->data(Qt::UserRole));

    const bool valid = m_formulaLength2->isValid();
    SetPointValidity(item, flagLength2, valid);

    if (valid)
    {
        const QString f = m_formulaLength2->formula();
        p.SetLength2(Visualization::FindLengthFromUser(f, data.DataVariables(), false), f);
        item->setData(Qt::UserRole, QVariant::fromValue(p));
    }

    ShowPointIssue(p.P().name());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SaveCurrentPoint()
{
    // Flush any still-debounced edits from the widgets into the current list item.
    Length1Changed();
    Angle1Changed();
    Length2Changed();
    Angle2Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SavePath()
{
    SaveCurrentPoint();

    const quint32 d = path.GetDuplicate(); // Save previous value
    path.Clear();
    path = ExtractPath();
    path.SetApproximationScale(ui->doubleSpinBoxApproximationScale->value());
    path.SetPenStyle(GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine));
    path.SetColor(GetColorComboCurrentName(m_colorCombo));
    path.SetAliasSuffix(ui->lineEditAlias->text());
    path.SetDuplicate(d);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::AllIds() const -> QSet<quint32>
{
    QSet<quint32> ids;
    for (qint32 i = 0; i < ui->listWidget->count(); ++i)
    {
        ids.insert(qvariant_cast<VSplinePoint>(ui->listWidget->item(i)->data(Qt::UserRole)).P().id());
    }

    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::IsPathValid() const -> bool
{
    if (path.CountPoints() < 3)
    {
        return false;
    }

    return (AllIds().size() == path.CountPoints());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::ExtractPath() const -> VSplinePath
{
    QVector<VSplinePoint> points;
    points.reserve(ui->listWidget->count());
    for (qint32 i = 0; i < ui->listWidget->count(); ++i)
    {
        points.append(qvariant_cast<VSplinePoint>(ui->listWidget->item(i)->data(Qt::UserRole)));
    }
    return VSplinePath(points);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::ShowPointIssue(const QString &pName)
{
    const int row = ui->listWidget->currentRow();
    if (row == -1)
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->item(row);
    SCASSERT(item != nullptr)

    if ((item->data(validityRole).toInt() & allHandleFlags) == allHandleFlags)
    {
        item->setText(pName);
    }
    else
    {
        item->setText(pName + "(!)"_L1);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveControls()
{
    const int index = ui->listWidget->currentRow();

    ui->toolButtonTop->setEnabled(index > 0);
    ui->toolButtonUp->setEnabled(index > 0);
    ui->toolButtonDown->setEnabled(index != -1 && index < ui->listWidget->count() - 1);
    ui->toolButtonBottom->setEnabled(index != -1 && index < ui->listWidget->count() - 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::IsValid() const -> bool
{
    if (not flagError || not flagAlias)
    {
        return false;
    }

    for (qint32 i = 0; i < ui->listWidget->count(); ++i)
    {
        QListWidgetItem const *item = ui->listWidget->item(i);
        if (item != nullptr && (item->data(validityRole).toInt() & allHandleFlags) != allHandleFlags)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event); // NOLINT(bugprone-parent-virtual-call)
    if (event->spontaneous())
    {
        return;
    }
    if (isInitialized)
    {
        return;
    }
    // do your init stuff here

    const QSize sz = VAbstractApplication::VApp()->Settings()->GetDialogSplinePathSize();
    if (not sz.isEmpty())
    {
        resize(sz);
    }

    isInitialized = true; // first show windows are held
    ShowVisualization();

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::resizeEvent(QResizeEvent *event)
{
    // remember the size for the next time this dialog is opened, but only
    // if widget was already initialized, which rules out the resize at
    // dialog creating, which would
    if (isInitialized)
    {
        VAbstractApplication::VApp()->Settings()->SetDialogSplinePathSize(size());
    }
    QDialog::resizeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SetDefPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SetDefColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::CheckDependencyTreeComplete()
{
    m_dependencyReady = m_doc->IsPatternGraphComplete();
    ui->toolButtonAddPoint->setEnabled(m_dependencyReady);
    ui->toolButtonRemovePoint->setEnabled(m_dependencyReady);
    ui->toolButtonTop->setEnabled(m_dependencyReady);
    ui->toolButtonUp->setEnabled(m_dependencyReady);
    ui->toolButtonDown->setEnabled(m_dependencyReady);
    ui->toolButtonBottom->setEnabled(m_dependencyReady);
    ui->lineEditAlias->setEnabled(m_dependencyReady);
}
