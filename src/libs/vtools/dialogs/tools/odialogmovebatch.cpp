/************************************************************************
 **
 **  @file   odialogmovebatch.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The dialog of a move by batch: one row per cut line, and the three values each one applies.
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

#include "odialogmovebatch.h"

#include <QColor>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSharedPointer>
#include <QTabBar>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../../visualization/line/operation/ovistoolmovebatch.h"
#include "../../visualization/visualization.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../widgets/angleformulawidget.h"
#include "../widgets/formulainputwidget.h"
#include "ui_odialogmovebatch.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto AddCell(QTableWidget *table, const QString &text, int row, int column) -> QTableWidgetItem *
{
    SCASSERT(table != nullptr)

    auto *item = new QTableWidgetItem(text);
    item->setToolTip(text);
    Qt::ItemFlags flags = item->flags();
    flags &= ~(Qt::ItemIsEditable);
    item->setFlags(flags);
    table->setItem(row, column, item);
    return item;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODialogMoveBatch::ODialogMoveBatch(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogMoveBatch)
{
    ui->setupUi(this);

    // The three values of a move are independent, and both angles are signed and free: a part may be carried
    // backwards, and it may turn either way. Nothing is derived from the pair of points -- it only says which
    // objects travel, and where the part turns.
    m_formulaLength = new FormulaInputWidget(this);
    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    m_formulaLength->setDialogTitle(tr("Edit length"));
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this, &ODialogMoveBatch::LengthChanged);

    m_formulaAngle = new AngleFormulaWidget(this);
    m_formulaAngle->setData(&this->data);
    m_formulaAngle->setToolId(toolId);
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &AngleFormulaWidget::valueChanged, this, &ODialogMoveBatch::AngleChanged);

    m_formulaRotationAngle = new AngleFormulaWidget(this);
    m_formulaRotationAngle->setData(&this->data);
    m_formulaRotationAngle->setToolId(toolId);
    m_formulaRotationAngle->setLabelText(tr("Rotation angle:"));
    m_formulaRotationAngle->setPostfix(degreeSymbol);
    m_formulaRotationAngle->setDialogTitle(tr("Edit rotation angle"));
    connect(m_formulaRotationAngle, &AngleFormulaWidget::valueChanged, this,
            &ODialogMoveBatch::RotationAngleChanged);

    InsertFormulaField(ui->formulaLengthPlaceholder, m_formulaLength);
    ui->formulaLengthPlaceholder = nullptr;
    InsertFormulaField(ui->formulaAnglePlaceholder, m_formulaAngle);
    ui->formulaAnglePlaceholder = nullptr;
    InsertFormulaField(ui->formulaRotationAnglePlaceholder, m_formulaRotationAngle);
    ui->formulaRotationAnglePlaceholder = nullptr;

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &ODialogMoveBatch::ShowMoveDetails);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &ODialogMoveBatch::SaveMoveName);
    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);

    InitOkCancelApply(ui);

    ui->tabWidget->setCurrentIndex(0);
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide();
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);

    vis = new OVisToolMoveBatch(&this->data);

    FillMoves();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogMoveBatch::~ODialogMoveBatch()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogMoveBatch::GetMoves() const -> QVector<OMoveBatchMove>
{
    return m_moves;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::SetMoves(const QVector<OMoveBatchMove> &moves)
{
    m_moves = moves;
    FillMoves();
    // The selectRow(0) inside FillMoves is signal-blocked: load the first move into the editor here, so the
    // dialog does not open on an empty row.
    ShowMoveDetails();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::ShowDialog(bool click)
{
    if (m_step == Step::Selecting && !click)
    {
        if (SourceObjects().isEmpty())
        {
            return;
        }

        m_step = Step::Start;
        m_currentMove = 0;

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        scene->clearSelection();

        scene->ToggleArcSegmentVisibility(false);
        scene->ToggleElArcSegmentVisibility(false);
        scene->ToggleSplineSegmentVisibility(false);
        scene->ToggleSplinePathSegmentVisibility(false);
        scene->ToggleArcHover(false);
        scene->ToggleElArcHover(false);
        scene->ToggleSplineHover(false);
        scene->ToggleSplinePathHover(false);
        VAbstractValApplication::VApp()->getSceneView()->AllowRubberBand(false);

        // One generation, one suffix, no number: a move-by-batch copy is named <source>__M.
        for (auto &item : SourceObjects())
        {
            if (item.id == NULL_ID)
            {
                continue;
            }

            try
            {
                const QSharedPointer<VGObject> object = data.GetGObject(item.id);
                if (object->getType() == GOType::Point)
                {
                    item.name = object->name() + "__M"_L1;
                }
                else
                {
                    item.name = data.GeometricObject<VAbstractCurve>(item.id)->HeadlessName() + "__M"_L1;
                }
            }
            catch (const VExceptionBadId &)
            {
                continue;
            }
        }

        emit ToolTip(tr("Move %1: click the <b>start point</b> of the cut (the hem), "
                        "or <b>%2</b> to finish")
                         .arg(1)
                         .arg(VModifierKey::EnterKey()));
        return;
    }

    if (m_step == Step::Start && !click)
    {
        // Enter with all pairs picked: open the editor.
        if (m_moves.isEmpty())
        {
            emit ToolTip(tr("Pick at least one cut first: a start point, then a pivot point"));
            return;
        }

        m_step = Step::Editing;
        FillMoves();
        setModal(true);
        emit ToolTip(QString());
        show();
        return;
    }

    if (m_step == Step::Pivot && !click)
    {
        // Enter while the current pair is incomplete: drop it and open the editor with the pairs that are whole.
        m_moves.removeAt(m_currentMove);
        if (m_moves.isEmpty())
        {
            emit ToolTip(tr("Pick at least one cut first: a start point, then a pivot point"));
            return;
        }

        m_step = Step::Editing;
        FillMoves();
        ShowMoveDetails(); // load the first move, so the editor does not open empty
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::ChosenObject(quint32 id, const SceneObject &type)
{
    if (m_step == Step::Selecting)
    {
        if (type == SceneObject::Line)
        {
            ToggleLineSource(id);
        }
        else
        {
            ToggleSourceObject(id);
        }

        if (auto *operation = qobject_cast<OVisToolMoveBatch *>(vis))
        {
            operation->SetObjects(SourceToObjects(SourceObjects()));
            operation->SetPreviewLines(PreviewLines());
            if (!VAbstractValApplication::VApp()->getCurrentScene()->items().contains(operation))
            {
                operation->VisualMode(NULL_ID);
            }
            else
            {
                operation->RefreshGeometry();
            }
        }
        return;
    }

    if (type != SceneObject::Point)
    {
        return; // A move pair is made of two points.
    }

    // The two points must belong to the ordered selection: they delimit a region of it.
    const bool isSource = std::any_of(SourceObjects().cbegin(), SourceObjects().cend(),
                                      [id](const SourceItem &item) { return item.id == id; });
    if (!isSource)
    {
        emit ToolTip(tr("This point is not one of the selected objects, pick a point of the contour"));
        return;
    }

    if (m_step == Step::Start)
    {
        m_moves.append({.startId = id,
                        .pivotId = NULL_ID,
                        .name = OMoveBatchRule::DefaultMoveName(m_currentMove),
                        .length = OMoveBatchRule::DefaultLength,
                        .angle = OMoveBatchRule::DefaultAngle,
                        .rotationAngle = OMoveBatchRule::DefaultRotationAngle});
        m_step = Step::Pivot;
        emit ToolTip(tr("Move %1: click its <b>pivot point</b> (the point the part turns around)")
                         .arg(m_currentMove + 1));
    }
    else if (m_step == Step::Pivot)
    {
        OMoveBatchMove &move = m_moves[m_currentMove];
        move.pivotId = id;

        // The pair is unordered: the role follows the position in the list (start before pivot).
        const int startIndex = OMoveBatchRule::IndexOf(SourceObjects(), move.startId);
        const int pivotIndex = OMoveBatchRule::IndexOf(SourceObjects(), move.pivotId);
        if (pivotIndex < startIndex)
        {
            std::swap(move.startId, move.pivotId);
        }

        m_currentMove++;
        m_step = Step::Start;
        emit ToolTip(tr("Move %1: click the <b>start point</b> of the cut (the hem), "
                        "or <b>%2</b> to finish")
                         .arg(m_currentMove + 1)
                         .arg(VModifierKey::EnterKey()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::OnSourceObjectsSet()
{
    if (auto *operation = qobject_cast<OVisToolMoveBatch *>(vis))
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::ShowVisualization()
{
    AddVisualization<OVisToolMoveBatch>();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::SaveData()
{
    // A value typed in the fields and never left (the fields evaluate on a timer) still has to become the value
    // that gets stored, before the editor is read back.
    CommitEditorFor(CurrentRow());

    if (auto *operation = qobject_cast<OVisToolMoveBatch *>(vis))
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->SetMoves(SourceToObjects(SourceObjects()), m_moves);
        operation->RefreshGeometry();
    }

    QStringList groupTags = ui->lineEditGroupTags->text().split(',');
    for (auto &tag : groupTags)
    {
        tag = tag.trimmed();
        if (not GroupTags().contains(tag))
        {
            GroupTags().append(tag);
        }
    }

    ui->lineEditGroupTags->SetCompletion(GroupTags());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        m_formulaLength->setLabelText(tr("Length:"));
        m_formulaAngle->setLabelText(tr("Angle:"));
        m_formulaRotationAngle->setLabelText(tr("Rotation angle:"));
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogMoveBatch::IsValid() const -> bool
{
    const bool hasMoves = !m_moves.isEmpty();
    const bool lengthOk = (m_formulaLength == nullptr) || m_formulaLength->isValid();
    const bool angleOk = (m_formulaAngle == nullptr) || m_formulaAngle->isValid();
    const bool rotationAngleOk = (m_formulaRotationAngle == nullptr) || m_formulaRotationAngle->isValid();
    const bool ready = hasMoves && lengthOk && angleOk && rotationAngleOk && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
        ui->labelStatus->setVisible(false);
    }
    else if (hasMoves && lengthOk && angleOk && rotationAngleOk)
    {
        ui->labelStatus->setText(tr("Invalid layer name"));
        ui->labelStatus->setVisible(true);
    }

    return ready;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogMoveBatch::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::ShowMoveDetails()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_moves.size())
    {
        EnableDetails(false);
        m_editorRow = -1;
        return;
    }

    // Write back what the fields hold for the row being left, before their content is replaced: the formula
    // fields evaluate on a timer, so an edit typed and not yet evaluated would otherwise be lost.
    CommitEditorFor(m_editorRow);

    EnableDetails(true);
    m_editorRow = row;

    const OMoveBatchMove &move = m_moves.at(row);

    {
        const QSignalBlocker blocker(ui->lineEditName); // textEdited only fires on a user edit
        ui->lineEditName->setText(move.name);
    }

    // No signal blocker on the formula fields: their evaluation is triggered by the editor's textChanged, so
    // blocking it would leave isValid()/value() stale.
    m_formulaLength->setFormula(move.length);
    m_formulaAngle->setFormula(move.angle);
    m_formulaRotationAngle->setFormula(move.rotationAngle);

    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::SaveMoveName(const QString &text)
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_moves.size())
    {
        return;
    }

    m_moves[row].name = text;

    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::LengthChanged(bool valid)
{
    if (!valid)
    {
        ShowError(tr("Invalid length formula"));
        CheckState();
        return;
    }

    CommitEditorFor(CurrentRow());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::AngleChanged(bool valid)
{
    if (!valid)
    {
        ShowError(tr("Invalid angle formula"));
        CheckState();
        return;
    }

    CommitEditorFor(CurrentRow());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::RotationAngleChanged(bool valid)
{
    if (!valid)
    {
        ShowError(tr("Invalid rotation angle formula"));
        CheckState();
        return;
    }

    CommitEditorFor(CurrentRow());
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogMoveBatch::CurrentRow() const -> int
{
    return ui->tableWidget->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::RefreshRow(int row)
{
    if (row < 0 || row >= m_moves.size())
    {
        return;
    }

    const QSignalBlocker blocker(ui->tableWidget);
    const OMoveBatchMove &move = m_moves.at(row);
    AddCell(ui->tableWidget, move.name, row, 0);
    AddCell(ui->tableWidget, move.length, row, 1);
    AddCell(ui->tableWidget, move.angle, row, 2);
    AddCell(ui->tableWidget, move.rotationAngle, row, 3);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::FillMoves()
{
    m_editorRow = -1; // the table is rebuilt: the editor no longer describes any of the previous rows

    const QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(static_cast<int>(m_moves.size()));

    for (int i = 0; i < m_moves.size(); ++i)
    {
        const OMoveBatchMove &move = m_moves.at(i);
        AddCell(ui->tableWidget, move.name, i, 0);
        AddCell(ui->tableWidget, move.length, i, 1);
        AddCell(ui->tableWidget, move.angle, i, 2);
        AddCell(ui->tableWidget, move.rotationAngle, i, 3);
    }

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    if (ui->tableWidget->rowCount() > 0)
    {
        ui->tableWidget->selectRow(0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::EnableDetails(bool enabled)
{
    ui->lineEditName->setEnabled(enabled);
    m_formulaLength->setEnabled(enabled);
    m_formulaAngle->setEnabled(enabled);
    m_formulaRotationAngle->setEnabled(enabled);

    if (!enabled)
    {
        const QSignalBlocker blockerName(ui->lineEditName);
        ui->lineEditName->clear();

        const QSignalBlocker blockerLength(m_formulaLength->editor());
        m_formulaLength->setFormula(QString());

        const QSignalBlocker blockerAngle(m_formulaAngle->editor());
        m_formulaAngle->setFormula(QString());

        const QSignalBlocker blockerRotationAngle(m_formulaRotationAngle->editor());
        m_formulaRotationAngle->setFormula(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::CommitEditorFor(int row)
{
    if (row < 0 || row >= m_moves.size())
    {
        return;
    }

    // The name goes with the three values: all the fields describe one row, and they are written back together.
    m_moves[row].name = ui->lineEditName->text();
    m_moves[row].length = m_formulaLength->formula();
    m_moves[row].angle = m_formulaAngle->formula();
    m_moves[row].rotationAngle = m_formulaRotationAngle->formula();

    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogMoveBatch::ShowError(const QString &message)
{
    ui->labelStatus->setText(message);
    ui->labelStatus->setVisible(true);
}
