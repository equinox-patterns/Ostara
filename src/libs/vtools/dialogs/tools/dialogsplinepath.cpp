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
#include <QHeaderView>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVariant>

#include "../../visualization/path/vistoolsplinepath.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../widgets/formulainputwidget.h"
#include "../widgets/angleformulawidget.h"
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
// A row carries its point and the validity of its four handle fields in the FIRST cell of the row, under these two
// roles. A set bit means the corresponding control-handle field is valid.
constexpr int pointRole = Qt::UserRole;
constexpr int validityRole = Qt::UserRole + 1;
constexpr int flagLength1 = 0x1;
constexpr int flagAngle1 = 0x2;
constexpr int flagLength2 = 0x4;
constexpr int flagAngle2 = 0x8;
constexpr int allHandleFlags = flagLength1 | flagAngle1 | flagLength2 | flagAngle2;

// The columns of the point table: the row itself, then the four handle values — the incoming handle of the point
// (Length 1, Angle 1) before its outgoing one (Length 2, Angle 2).
constexpr int columnPoint = 0;
constexpr int columnLength1 = 1;
constexpr int columnAngle1 = 2;
constexpr int columnLength2 = 3;
constexpr int columnAngle2 = 4;
constexpr int pointColumnCount = 5;

// What a handle the point does not have shows in its two cells — the dash the pleat table uses for an empty value.
auto NotUsedCell() -> QString
{
    return QStringLiteral("-");
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

    m_formulaAngle1 = new AngleFormulaWidget(this);
    m_formulaAngle1->setData(&this->data);
    m_formulaAngle1->setToolId(toolId);
    m_formulaAngle1->setLabelText(tr("Angle:"));
    m_formulaAngle1->setPostfix(degreeSymbol);
    m_formulaAngle1->setDialogTitle(tr("Edit first control point angle"));
    connect(m_formulaAngle1, &AngleFormulaWidget::valueChanged, this, [this]() { Angle1Changed(); });

    m_formulaLength2 = new FormulaInputWidget(this);
    m_formulaLength2->setData(&this->data);
    m_formulaLength2->setToolId(toolId);
    m_formulaLength2->setLabelText(tr("Length:"));
    m_formulaLength2->setPostfix(lengthPostfix);
    m_formulaLength2->setDialogTitle(tr("Edit second control point length"));
    m_formulaLength2->setCheckLessThanZero(true);
    connect(m_formulaLength2, &FormulaInputWidget::valueChanged, this, [this]() { Length2Changed(); });

    m_formulaAngle2 = new AngleFormulaWidget(this);
    m_formulaAngle2->setData(&this->data);
    m_formulaAngle2->setToolId(toolId);
    m_formulaAngle2->setLabelText(tr("Angle:"));
    m_formulaAngle2->setPostfix(degreeSymbol);
    m_formulaAngle2->setDialogTitle(tr("Edit second control point angle"));
    connect(m_formulaAngle2, &AngleFormulaWidget::valueChanged, this, [this]() { Angle2Changed(); });

    // Replace placeholders
    auto replacePlaceholder = [this](const char *name, QWidget *widget)
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

    // The rows are the points, in the order the path is walked: "Point 1" is the start, the last row the end. The
    // table shows them, the fields below edit the selected one — a cell is never edited in place (Pleat's table).
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setMinimumHeight(90);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // A table has no currentRowChanged of its own — the current cell carries the row.
    connect(ui->tableWidget, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { PointChanged(row); });
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
        const QSignalBlocker blocker(ui->tableWidget);
        ui->tableWidget->setRowCount(0);
        for (qint32 i = 0; i < path.CountPoints(); ++i)
        {
            NewItem(path.at(i));
        }
        ui->tableWidget->setFocus(Qt::OtherFocusReason);

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

    ui->toolButtonRemovePoint->setEnabled(ui->tableWidget->rowCount() > 3);
    RefreshRows();

    // The rows were filled with the table's signals blocked, so the row that is current — the one the dialog opens
    // on — still has to be shown as the active point of the path.
    SetActivePoint(CurrentRow());

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
    if (ui->tableWidget->rowCount() == 0 || row < 0)
    {
        ui->toolButtonTop->setEnabled(false);
        ui->toolButtonUp->setEnabled(false);
        ui->toolButtonDown->setEnabled(false);
        ui->toolButtonBottom->setEnabled(false);
        SetActivePoint(-1);
        return;
    }

    MoveControls();

    DataPoint(RowPoint(row));

    // The row being edited is the point the canvas shows as active.
    SetActivePoint(row);
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

    const int row = CurrentRow();
    if (row < 0)
    {
        return;
    }

    try
    {
        auto p = RowPoint(row);

        const auto point = data.GeometricObject<VPointF>(id);
        p.SetP(*point);

        DataPoint(p);
        SetRowPoint(row, p);
        RefreshRow(row);

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
    if (ui->comboBoxNewPoint->currentIndex() == -1)
    {
        return; // Nothing selected. Can happen if a pattern change re-enabled the button (see
                // CheckDependencyTreeComplete()) while the combo box was empty.
    }

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
    if (ui->tableWidget->rowCount() <= 3)
    {
        ui->toolButtonRemovePoint->setDisabled(true);
        return;
    }

    const int row = CurrentRow();
    if (row >= 0)
    {
        ui->tableWidget->removeRow(row);
        RefreshRows();
        ui->tableWidget->setCurrentCell(qMin(row, ui->tableWidget->rowCount() - 1), columnPoint);
        ui->toolButtonRemovePoint->setDisabled(ui->tableWidget->rowCount() <= 3);
    }

    SavePath();

    flagError = IsPathValid();
    CheckState(); // Disable Ok and Apply buttons if something wrong.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveTop()
{
    MoveRow(CurrentRow(), 0);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveUp()
{
    const int row = CurrentRow();
    MoveRow(row, row - 1);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveDown()
{
    const int row = CurrentRow();
    MoveRow(row, row + 1);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveBottom()
{
    MoveRow(CurrentRow(), ui->tableWidget->rowCount() - 1);
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
    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    CreateRow(row);
    SetRowPoint(row, point);
    SetRowValidity(row, allHandleFlags);
    RefreshRow(row);

    ui->tableWidget->setCurrentCell(row, columnPoint);
    if (ui->tableWidget->rowCount() >= 2)
    {
        bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
        bOk->setEnabled(true);
        ui->toolButtonRemovePoint->setEnabled(true);
    }

    DataPoint(point);

    // A row filled while the table was still growing took itself for the last one: the row that was the end until
    // now has an outgoing handle no more.
    RefreshRows();
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

    int const row = CurrentRow();
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
    else if (row == ui->tableWidget->rowCount() - 1)
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

    const int row = CurrentRow();
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    auto p = RowPoint(row);

    const bool valid = m_formulaAngle1->isValid();
    UpdateRowValidity(row, flagAngle1, valid);

    if (valid)
    {
        const QString f = m_formulaAngle1->formula();
        const qreal value = Visualization::FindValFromUser(f, data.DataVariables(), false);
        if (not VFuzzyComparePossibleNulls(value, p.Angle1()))
        {
            p.SetAngle1(value, f);
            SetRowPoint(row, p);

            // Strict coupling: the opposite angle follows. Reflect it in the UI.
            if (row != ui->tableWidget->rowCount() - 1)
            {
                m_formulaAngle2->setFormula(p.Angle2Formula());
                UpdateRowValidity(row, flagAngle2, true);
            }
        }
    }

    RefreshRow(row);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Angle2Changed()
{
    if (m_formulaAngle2 == nullptr || not m_formulaAngle2->isEnabled())
    {
        return;
    }

    const int row = CurrentRow();
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    auto p = RowPoint(row);

    const bool valid = m_formulaAngle2->isValid();
    UpdateRowValidity(row, flagAngle2, valid);

    if (valid)
    {
        const QString f = m_formulaAngle2->formula();
        const qreal value = Visualization::FindValFromUser(f, data.DataVariables(), false);
        if (not VFuzzyComparePossibleNulls(value, p.Angle2()))
        {
            p.SetAngle2(value, f);
            SetRowPoint(row, p);

            if (row != 0)
            {
                m_formulaAngle1->setFormula(p.Angle1Formula());
                UpdateRowValidity(row, flagAngle1, true);
            }
        }
    }

    RefreshRow(row);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Length1Changed()
{
    if (m_formulaLength1 == nullptr || not m_formulaLength1->isEnabled())
    {
        return;
    }

    const int row = CurrentRow();
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    auto p = RowPoint(row);

    const bool valid = m_formulaLength1->isValid();
    UpdateRowValidity(row, flagLength1, valid);

    if (valid)
    {
        const QString f = m_formulaLength1->formula();
        p.SetLength1(Visualization::FindLengthFromUser(f, data.DataVariables(), false), f);
        SetRowPoint(row, p);
    }

    RefreshRow(row);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::Length2Changed()
{
    if (m_formulaLength2 == nullptr || not m_formulaLength2->isEnabled())
    {
        return;
    }

    const int row = CurrentRow();
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    auto p = RowPoint(row);

    const bool valid = m_formulaLength2->isValid();
    UpdateRowValidity(row, flagLength2, valid);

    if (valid)
    {
        const QString f = m_formulaLength2->formula();
        p.SetLength2(Visualization::FindLengthFromUser(f, data.DataVariables(), false), f);
        SetRowPoint(row, p);
    }

    RefreshRow(row);
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
    const qreal approximationScale = path.GetApproximationScale(); // Preserve across rebuild
    path.Clear();
    path = ExtractPath();
    path.SetApproximationScale(approximationScale);
    path.SetPenStyle(GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine));
    path.SetColor(GetColorComboCurrentName(m_colorCombo));
    path.SetAliasSuffix(ui->lineEditAlias->text());
    path.SetDuplicate(d);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::AllIds() const -> QSet<quint32>
{
    QSet<quint32> ids;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        ids.insert(RowPoint(i).P().id());
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
    points.reserve(ui->tableWidget->rowCount());
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        points.append(RowPoint(i));
    }
    return VSplinePath(points);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief CreateRow gives a row its five cells: empty, non-editable, and carrying nothing yet.
 *
 *  What the row means lives in its first cell — the point itself and the validity of its four handle fields — so
 *  that the row is the model and the table only shows it. */
void DialogSplinePath::CreateRow(int row)
{
    for (int column = 0; column < pointColumnCount; ++column)
    {
        auto *cell = new QTableWidgetItem();
        cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, column, cell);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::RowPoint(int row) const -> VSplinePoint
{
    const QTableWidgetItem *cell = ui->tableWidget->item(row, columnPoint);
    return cell != nullptr ? qvariant_cast<VSplinePoint>(cell->data(pointRole)) : VSplinePoint();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SetRowPoint(int row, const VSplinePoint &p)
{
    if (QTableWidgetItem *cell = ui->tableWidget->item(row, columnPoint); cell != nullptr)
    {
        cell->setData(pointRole, QVariant::fromValue(p));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::RowValidity(int row) const -> int
{
    const QTableWidgetItem *cell = ui->tableWidget->item(row, columnPoint);
    return cell != nullptr ? cell->data(validityRole).toInt() : allHandleFlags;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::SetRowValidity(int row, int validity)
{
    if (QTableWidgetItem *cell = ui->tableWidget->item(row, columnPoint); cell != nullptr)
    {
        cell->setData(validityRole, validity);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::UpdateRowValidity(int row, int flag, bool valid)
{
    int validity = RowValidity(row);
    if (valid)
    {
        validity |= flag;
    }
    else
    {
        validity &= ~flag;
    }
    SetRowValidity(row, validity);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief RefreshRow writes a row back from its own data: its rank as a name — never the name of the point, which is
 *  what the pattern calls it and not what the path does with it — and the four values of its two handles.
 *
 *  A point at either end of the path has one handle only: the columns of the handle it does not have show a dash,
 *  the way the fields below are greyed and read "Not used". A row whose values do not all hold says so with "(!)". */
void DialogSplinePath::RefreshRow(int row)
{
    if (row < 0 || row >= ui->tableWidget->rowCount())
    {
        return;
    }

    const VSplinePoint p = RowPoint(row);
    const bool first = row == 0;
    const bool last = row == ui->tableWidget->rowCount() - 1;

    auto setCell = [this, row](int column, const QString &formula, bool used)
    {
        QTableWidgetItem *cell = ui->tableWidget->item(row, column);
        if (cell != nullptr)
        {
            cell->setText(used ? VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                                     formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator())
                               : NotUsedCell());
        }
    };

    setCell(columnPoint, PointLabel(row), true);
    setCell(columnLength1, p.Length1Formula(), not first);
    setCell(columnAngle1, p.Angle1Formula(), not first);
    setCell(columnLength2, p.Length2Formula(), not last);
    setCell(columnAngle2, p.Angle2Formula(), not last);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief PointLabel is the rank of a row, with the mark of the rows whose values do not all hold. */
auto DialogSplinePath::PointLabel(int row) const -> QString
{
    QString label = tr("Point %1").arg(row + 1);
    if ((RowValidity(row) & allHandleFlags) != allHandleFlags)
    {
        label += "(!)"_L1;
    }
    return label;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief RefreshRows rewrites every row from its own data.
 *
 *  Whether a point has an incoming or an outgoing handle depends on its RANK: adding, removing or moving a point
 *  changes which rows are the two ends, and a row filled while the table was still growing cannot know it. */
void DialogSplinePath::RefreshRows()
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        RefreshRow(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief MoveRow moves a point to another rank, keeping the order of the rows it crosses: a path is walked, so
 *  moving a point is not swapping two of them. */
void DialogSplinePath::MoveRow(int from, int to)
{
    if (from < 0 || to < 0 || from == to || from >= ui->tableWidget->rowCount() ||
        to >= ui->tableWidget->rowCount())
    {
        return;
    }

    const VSplinePoint p = RowPoint(from);
    const int validity = RowValidity(from);

    ui->tableWidget->removeRow(from);
    ui->tableWidget->insertRow(to);
    CreateRow(to);
    SetRowPoint(to, p);
    SetRowValidity(to, validity);
    RefreshRow(to);
    RefreshRows();

    ui->tableWidget->setCurrentCell(to, columnPoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::CurrentRow() const -> int
{
    return ui->tableWidget->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief SetActivePoint shows which point of the path is being edited: the canvas paints it in the orange of the
 *  active point, so that the row and the point it stands for are read as one. */
void DialogSplinePath::SetActivePoint(int row)
{
    auto *visPath = qobject_cast<VisToolSplinePath *>(vis);
    if (visPath != nullptr)
    {
        visPath->SetActivePoint(row);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void DialogSplinePath::MoveControls()
{
    const int index = CurrentRow();

    ui->toolButtonTop->setEnabled(index > 0);
    ui->toolButtonUp->setEnabled(index > 0);
    ui->toolButtonDown->setEnabled(index != -1 && index < ui->tableWidget->rowCount() - 1);
    ui->toolButtonBottom->setEnabled(index != -1 && index < ui->tableWidget->rowCount() - 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSplinePath::IsValid() const -> bool
{
    if (not flagError || not flagAlias)
    {
        return false;
    }

    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        if ((RowValidity(i) & allHandleFlags) != allHandleFlags)
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

    // The visualization is built on show: the active point has to be given to it once it exists.
    SetActivePoint(CurrentRow());

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
    ui->toolButtonAddPoint->setEnabled(m_dependencyReady && ui->comboBoxNewPoint->currentIndex() != -1);
    ui->toolButtonRemovePoint->setEnabled(m_dependencyReady && ui->tableWidget->rowCount() > 3);
    ui->toolButtonTop->setEnabled(m_dependencyReady);
    ui->toolButtonUp->setEnabled(m_dependencyReady);
    ui->toolButtonDown->setEnabled(m_dependencyReady);
    ui->toolButtonBottom->setEnabled(m_dependencyReady);
    ui->lineEditAlias->setEnabled(m_dependencyReady);
}
