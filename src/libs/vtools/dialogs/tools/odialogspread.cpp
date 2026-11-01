/************************************************************************
 **
 **  @file   odialogspread.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The dialog of the slash-and-spread tool.
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

#include "odialogspread.h"

#include <QScreen>
#include <QTimer>


#include "../../tools/drawTools/operation/otoolspread.h"
#include "../../tools/drawTools/operation/ospreadtransform.h"

#include "../../tools/drawTools/operation/orotationlength.h"

#include <QColor>
#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSharedPointer>
#include <QTabBar>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../../visualization/line/operation/ovistoolspread.h"
#include "../../visualization/visualization.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/FactoryWidgets.h"
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
#include "../dialogtoolbox.h"
#include "ui_odialogspread.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
// A pen combo is wide enough to show its dash pattern, a colour combo only holds a swatch.
constexpr int kPenIconWidth = 80;
constexpr int kPenIconHeight = 14;
constexpr int kColorIconSize = 20;

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
ODialogSpread::ODialogSpread(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogSpread)
{
    ui->setupUi(this);

    // Angle and length say the same rotation, like the Rotation dialog: the radius is the distance pivot -> start
    // of the active rotation, so the length of a row is its own chord, not a shared one.
    m_formulaAngle = new AngleFormulaWidget(this);
    m_formulaAngle->setData(&this->data);
    m_formulaAngle->setToolId(toolId);
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &AngleFormulaWidget::valueChanged, this, &ODialogSpread::AngleChanged);

    m_formulaLength = new FormulaInputWidget(this);
    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    m_formulaLength->setDialogTitle(tr("Edit length"));
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this, &ODialogSpread::LengthChanged);

    InsertFormulaField(ui->formulaAnglePlaceholder, m_formulaAngle);
    ui->formulaAnglePlaceholder = nullptr;
    InsertFormulaField(ui->formulaLengthPlaceholder, m_formulaLength);
    ui->formulaLengthPlaceholder = nullptr;

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &ODialogSpread::ShowRotationDetails);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &ODialogSpread::SaveRotationName);
    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);
    // The length the smoothing costs is read while the figure is being built: the switch, the angles and the
    // rotations all change it, and the summary under the checkbox is what says by how much.
    connect(ui->checkBoxSmooth, &QCheckBox::toggled, this, &ODialogSpread::UpdateSmoothingSummary);

    // The pen of the display's lines, and their colour: one style for every line of every rotation. The combos start
    // on the mark's own default — orange, long dashes — and NOT on the layer the display will join: a mark is a
    // construction line, and it has to read as one whatever the layer happens to draw with. The user changes it if
    // he wants to, the dialog remembers what he chose (see the display of the operation).
    const QString layerPen = TypeLineDashLine;

    m_displayTypeLine = MakeComboBox(this);
    m_displayTypeLine->setMinimumWidth(kPenIconWidth);
    m_displayTypeLine->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_displayTypeLine->setIconSize(QSize(kPenIconWidth, kPenIconHeight));
    FillComboBoxTypeLine(m_displayTypeLine,
                         LineStylesPics(m_displayTypeLine->palette().color(QPalette::Base),
                                        m_displayTypeLine->palette().color(QPalette::Text)),
                         StylesList().contains(layerPen) ? layerPen : TypeLineLine);

    m_displayColor = MakeComboBox(this);
    m_displayColor->setMinimumWidth(kPenIconWidth);
    m_displayColor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_displayColor->setIconSize(QSize(kColorIconSize, kColorIconSize));
    InitColorCombo(m_displayColor);
    SetColorComboCurrentName(m_displayColor, ColorOrange);

    ui->formLayoutDisplay->addRow(tr("Line type:"), m_displayTypeLine);
    ui->formLayoutDisplay->addRow(tr("Line color:"), m_displayColor);

    InitOkCancelApply(ui);

    ui->tabWidget->setCurrentIndex(0);
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide();
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);

    vis = new OVisToolSpread(&this->data);

    FillRotations();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogSpread::~ODialogSpread()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::GetRotations() const -> QVector<OSpreadRotation>
{
    return m_rotations;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SetRotations(const QVector<OSpreadRotation> &rotations)
{
    m_rotations = rotations;
    FillRotations();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::GetDisplay() const -> OSpreadDisplay
{
    OSpreadDisplay display;
    display.show = ui->groupBoxDisplay->isChecked();
    display.typeLine = GetComboBoxCurrentData(m_displayTypeLine, TypeLineLine);
    display.lineColor = GetColorComboCurrentName(m_displayColor);
    return display;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SetDisplay(const OSpreadDisplay &display)
{
    ui->groupBoxDisplay->setChecked(display.show);
    ChangeCurrentData(m_displayTypeLine, display.typeLine);
    SetColorComboCurrentName(m_displayColor, display.lineColor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::IsSmooth() const -> bool
{
    return ui->checkBoxSmooth->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SetSmooth(bool smooth)
{
    ui->checkBoxSmooth->setChecked(smooth);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (m_sized)
    {
        return; // The user's own size and place are theirs once the dialogue has been shown.
    }
    m_sized = true;

    const QScreen *current = screen();
    if (current == nullptr)
    {
        return;
    }

    // Four fifths of the room the screen offers: tall enough to work in, never taller than what the screen has.
    // What does not fit scrolls, and the buttons are outside the scrolling, so they stay where they are. The
    // maximum is what makes the promise hold: the layout can ask for more, it cannot get it.
    const QRect available = current->availableGeometry();
    setMaximumHeight(available.height() - 40);
    resize(width(), qBound(minimumHeight(), available.height() * 4 / 5, available.height() - 40));

    // And it has to sit where the screen is. The window manager places the dialogue *after* it is shown, and
    // does it again a moment later: the place is therefore asserted three times, the last one once the window
    // is really on screen. Every call is a no-op when the dialogue is already whole.
    KeepOnScreen();
    QTimer::singleShot(0, this, &ODialogSpread::KeepOnScreen);
    QTimer::singleShot(150, this, &ODialogSpread::KeepOnScreen);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::KeepOnScreen()
{
    const QScreen *current = screen();
    if (current == nullptr)
    {
        return;
    }

    const QRect available = current->availableGeometry();
    const int margin = 20;

    // A dialogue taller than the screen cannot be kept on it: it is made to fit first.
    if (height() > available.height() - 2 * margin)
    {
        resize(width(), available.height() - 2 * margin);
    }

    // The dialogue is CENTRED on the screen rather than pushed up until it fits: a dialogue sitting on the
    // bottom edge is as unusable as one under it, and the user has to look for its buttons every time.
    const QRect frame = frameGeometry();
    const QPoint clientOffset = pos() - frame.topLeft();
    const QPoint target
        = available.center() - QPoint(frame.width() / 2, frame.height() / 2) + clientOffset;
    move(target);
}


//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::UpdateSmoothingSummary()
{
    const bool on = IsSmooth();
    ui->labelSmoothingSummary->setVisible(on);
    if (not on)
    {
        ui->labelSmoothingSummary->clear();
        return;
    }

    QVector<qreal> angles;
    angles.reserve(m_rotations.size());
    for (const auto &rotation : std::as_const(m_rotations))
    {
        angles.append(VisLine::FindValFromUser(rotation.angle, data.DataVariables()));
    }

    // Rich text: one line per curve, opened by the dot of the colour that identifies it — the same dot the property
    // browser shows when a copy is clicked, a name being no help here (the curves are "Curve 1", "Curve 2").
    const QString text = OToolSpread::SmoothingText(&data, SourceObjects(), m_rotations, angles);
    ui->labelSmoothingSummary->setText(OToolSpread::SmoothingHtml(&data, SourceObjects(), m_rotations, angles));
    ui->labelSmoothingSummary->setToolTip(text);

    // A line per curve: a long selection would make the row as tall as the dialogue. Four lines are enough to
    // see what the smoothing costs, the rest is in the tooltip.
    const int line = ui->labelSmoothingSummary->fontMetrics().lineSpacing();
    ui->labelSmoothingSummary->setMaximumHeight(line * 4);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::CopySuffixName() const -> QString
{
    const QString suffix = ui->lineEditCopySuffix->text().trimmed();
    return suffix.isEmpty() ? QStringLiteral("_Rotation") : suffix;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SetCopySuffixName(const QString &suffix)
{
    ui->lineEditCopySuffix->setText(suffix);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::ShowDialog(bool click)
{
    if (m_step == Step::Selecting && !click)
    {
        if (SourceObjects().isEmpty())
        {
            return;
        }

        m_step = Step::Start;
        m_currentRotation = 0;

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

        // One generation, one suffix, no number: a slash-and-spread copy is named <source>__R.
        for (auto &item : SourceObjects())
        {
            if (item.id == NULL_ID)
            {
                continue;
            }

            try
            {
                // A point keeps the mark of the operation, "__R"; a curve or an arc is named after its source and
                // the suffix of the dialog. The tool applies the same rule when it saves (see OToolSpread).
                const QSharedPointer<VGObject> object = data.GetGObject(item.id);
                const bool point = object.isNull() || object->getType() == GOType::Point;
                item.name = point ? DefaultCopyName(&data, item.id, CopySuffix::Spread)
                                  : CopyNameOfSource(&data, item.id, CopySuffixName());
            }
            catch (const VExceptionBadId &)
            {
                continue;
            }
        }

        emit ToolTip(tr("Rotation %1: click the <b>start point</b> of the cut (the hem), "
                        "or <b>%2</b> to finish")
                         .arg(1)
                         .arg(VModifierKey::EnterKey()));
        return;
    }

    if (m_step == Step::Start && !click)
    {
        // Enter with all pairs picked: open the editor.
        if (m_rotations.isEmpty())
        {
            emit ToolTip(tr("Pick at least one cut first: a start point, then a pivot point"));
            return;
        }

        m_step = Step::Editing;
        FillRotations();
        setModal(true);
        emit ToolTip(QString());
        show();
        return;
    }

    if (m_step == Step::Pivot && !click)
    {
        // Enter while the current pair is incomplete: drop it and open the editor with the pairs that are whole.
        m_rotations.removeAt(m_currentRotation);
        if (m_rotations.isEmpty())
        {
            emit ToolTip(tr("Pick at least one cut first: a start point, then a pivot point"));
            return;
        }

        m_step = Step::Editing;
        FillRotations();
        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::ChosenObject(quint32 id, const SceneObject &type)
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

        if (auto *operation = ConfiguredVis())
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
        return; // A rotation pair is made of two points.
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
        m_rotations.append({.startId = id,
                            .pivotId = NULL_ID,
                            .name = OSpreadRule::DefaultRotationName(m_currentRotation),
                            .angle = OSpreadRule::DefaultAngle});
        m_step = Step::Pivot;
        emit ToolTip(tr("Rotation %1: click its <b>pivot point</b> (the apex the part turns around)")
                         .arg(m_currentRotation + 1));
    }
    else if (m_step == Step::Pivot)
    {
        // A cut line joins two DIFFERENT points of the selection: the same point twice describes no region, and
        // the rule would skip the pair without a word — a cut in the list that cuts nothing.
        if (id == m_rotations[m_currentRotation].startId)
        {
            emit ToolTip(tr("You clicked the same point twice."));
            return;
        }

        OSpreadRotation &rotation = m_rotations[m_currentRotation];
        rotation.pivotId = id;

        // The pair is unordered: the role follows the position in the list (start before pivot).
        const int startIndex = OSpreadRule::IndexOf(SourceObjects(), rotation.startId);
        const int pivotIndex = OSpreadRule::IndexOf(SourceObjects(), rotation.pivotId);
        if (pivotIndex < startIndex)
        {
            std::swap(rotation.startId, rotation.pivotId);
        }

        m_currentRotation++;
        m_step = Step::Start;
        emit ToolTip(tr("Rotation %1: click the <b>start point</b> of the cut (the hem), "
                        "or <b>%2</b> to finish")
                         .arg(m_currentRotation + 1)
                         .arg(VModifierKey::EnterKey()));
    }

    // The pair is drawn as it is picked, before the operation exists: green on the start point, red on the pivot
    // (the pleat's two colours). Without this push the gesture is invisible — the preview only learned the cuts when
    // the dialog opened, so the point just clicked left nothing on the canvas.
    if (auto *operation = ConfiguredVis())
    {
        operation->SetRotations(SourceToObjects(SourceObjects()), m_rotations);
        operation->SetSmooth(IsSmooth());
        operation->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::ConfiguredVis() -> OVisToolSpread *
{
    auto *operation = qobject_cast<OVisToolSpread *>(vis);
    if (operation != nullptr)
    {
        operation->SetInPattern(this->toolId != NULL_ID);
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::OnSourceObjectsSet()
{
    // The summary reads the objects of the selection and the angles: both are in place when this is called, at
    // the end of the setup of the dialog as well as after a change of selection.
    UpdateSmoothingSummary();

    if (auto *operation = ConfiguredVis())
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, and the values the dialog shows follow.
        vis = new OVisToolSpread(&this->data);
    }

    AddVisualization<OVisToolSpread>();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SaveData()
{
    UpdateSmoothingSummary();

    // A length typed in the field and never left (the field evaluates on a timer) still has to become the angle
    // that gets stored, before the editor is read back.
    UpdateAngleFromLength();
    CommitCurrentAngle();

    if (auto *operation = ConfiguredVis())
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->SetRotations(SourceToObjects(SourceObjects()), m_rotations);
        operation->SetSmooth(IsSmooth());
        operation->RefreshGeometry();
    }

}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        m_formulaAngle->setLabelText(tr("Angle:"));
        m_formulaLength->setLabelText(tr("Length:"));
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::IsValid() const -> bool
{
    const bool hasRotations = !m_rotations.isEmpty();
    const bool angleOk = (m_formulaAngle == nullptr) || m_formulaAngle->isValid();
    const bool ready = hasRotations && angleOk && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
        ui->labelStatus->setVisible(false);
    }
    else if (hasRotations && angleOk)
    {
        ui->labelStatus->setText(tr("Invalid layer name"));
        ui->labelStatus->setVisible(true);
    }

    return ready;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::ShowRotationDetails()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_rotations.size())
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

    const OSpreadRotation &rotation = m_rotations.at(row);

    {
        const QSignalBlocker blocker(ui->lineEditName); // textEdited only fires on a user edit
        ui->lineEditName->setText(rotation.name);
    }

    // No signal blocker on the formula field: its evaluation is triggered by the editor's textChanged, so
    // blocking it would leave isValid()/value() stale, and the Length would never be computed. The echo it
    // sends back (valueChanged → AngleChanged) writes the same value to the same row.
    m_formulaAngle->setFormula(rotation.angle);

    // The field evaluates on a timer, so the length cannot be read back right here: it is computed when the
    // evaluation lands (AngleChanged → UpdateLengthFromAngle). The row itself is refreshed from the model now.
    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::SaveRotationName(const QString &text)
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_rotations.size())
    {
        return;
    }

    m_rotations[row].name = text;

    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::AngleChanged(bool valid)
{
    if (!valid)
    {
        ShowError(tr("Invalid angle formula"));
        CheckState();
        return;
    }

    CommitCurrentAngle();
    UpdateLengthFromAngle();
    UpdateSmoothingSummary();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::LengthChanged(bool valid)
{
    if (!valid)
    {
        ShowError(tr("Invalid length formula"));
        CheckState();
        return;
    }

    UpdateAngleFromLength();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::CurrentRow() const -> int
{
    return ui->tableWidget->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::RadiusFor(const OSpreadRotation &rotation) const -> qreal
{
    return ORotationLength::Radius(&data, rotation.pivotId, rotation.startId);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogSpread::LengthTextFor(const OSpreadRotation &rotation) -> QString
{
    const qreal radius = RadiusFor(rotation);
    if (!ORotationLength::HasRadius(radius))
    {
        // The two points of the pair are the same, or one of them is gone: there is no length to speak of.
        return QString();
    }

    VFormula fAngle(rotation.angle, &data);
    fAngle.setToolId(GetToolId());
    fAngle.Eval();
    if (fAngle.error())
    {
        return QString();
    }

    return FormatDisplayNumber(
        ORotationLength::LengthOfAngle(radius, fAngle.getDoubleValue(), *data.GetPatternUnit()));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::RefreshRow(int row)
{
    if (row < 0 || row >= m_rotations.size())
    {
        return;
    }

    const QSignalBlocker blocker(ui->tableWidget);
    const OSpreadRotation &rotation = m_rotations.at(row);
    AddCell(ui->tableWidget, rotation.name, row, 0);
    AddCell(ui->tableWidget, rotation.angle, row, 1);
    AddCell(ui->tableWidget, LengthTextFor(rotation), row, 2);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::FillRotations()
{
    m_editorRow = -1; // the table is rebuilt: the editor no longer describes any of the previous rows

    const QSignalBlocker blocker(ui->tableWidget);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(static_cast<int>(m_rotations.size()));

    for (int i = 0; i < m_rotations.size(); ++i)
    {
        const OSpreadRotation &rotation = m_rotations.at(i);
        AddCell(ui->tableWidget, rotation.name, i, 0);
        AddCell(ui->tableWidget, rotation.angle, i, 1);
        AddCell(ui->tableWidget, LengthTextFor(rotation), i, 2);
    }

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    if (ui->tableWidget->rowCount() > 0)
    {
        ui->tableWidget->selectRow(0);
    }

    // The editor describes the row the table has just selected. The selection signal is blocked while the table is
    // rebuilt, so nothing else loads it, and an editor left empty writes that emptiness into the row at the first
    // selection move (see ShowRotationDetails): the name of the first rotation would go away on its own. Loaded
    // here, so no caller has to remember it.
    ShowRotationDetails();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::EnableDetails(bool enabled)
{
    ui->lineEditName->setEnabled(enabled);
    m_formulaAngle->setEnabled(enabled);
    m_formulaLength->setEnabled(enabled);

    if (!enabled)
    {
        const QSignalBlocker blockerName(ui->lineEditName);
        ui->lineEditName->clear();

        const QSignalBlocker blockerAngle(m_formulaAngle->editor());
        m_formulaAngle->setFormula(QString());

        const QSignalBlocker blockerLength(m_formulaLength->editor());
        m_formulaLength->setFormula(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::UpdateLengthFromAngle()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_rotations.size() || !m_formulaAngle->isValid())
    {
        return;
    }

    const qreal radius = RadiusFor(m_rotations.at(row));
    if (!ORotationLength::HasRadius(radius))
    {
        return;
    }

    const qreal length = ORotationLength::LengthOfAngle(radius, m_formulaAngle->value(), *data.GetPatternUnit());

    m_lengthIsPossible = true;
    m_derivedLength = length;
    m_derivedLengthSet = true;

    if (m_formulaLength->isValid() && qAbs(m_formulaLength->value() - length) <= ORotationLength::DisplayTolerance)
    {
        return;
    }

    m_formulaLength->setFormula(FormatDisplayNumber(length));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::UpdateAngleFromLength()
{
    if (!m_formulaLength->isValid())
    {
        return;
    }

    const qreal typedLength = m_formulaLength->value();

    if (m_formulaAngle->isValid() && m_derivedLengthSet
        && qAbs(typedLength - m_derivedLength) <= ORotationLength::DisplayTolerance)
    {
        m_lengthIsPossible = true;
        return;
    }

    const int row = CurrentRow();
    if (row < 0 || row >= m_rotations.size())
    {
        return;
    }

    const qreal radius = RadiusFor(m_rotations.at(row));
    const qreal angle = ORotationLength::AngleOfLength(radius, typedLength, *data.GetPatternUnit());

    m_lengthIsPossible = !qIsNaN(angle);
    if (!m_lengthIsPossible)
    {
        ShowError(tr("The length cannot exceed the circle diameter (%1%2).")
                      .arg(ORotationLength::DiameterText(radius, *data.GetPatternUnit()),
                           UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
        return;
    }

    if (m_formulaAngle->isValid() && qAbs(m_formulaAngle->value() - angle) <= ORotationLength::DisplayTolerance)
    {
        return;
    }

    m_formulaAngle->setFormula(FormatDisplayNumber(angle));
    CommitCurrentAngle();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::CommitCurrentAngle()
{
    CommitEditorFor(CurrentRow());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::CommitEditorFor(int row)
{
    if (row < 0 || row >= m_rotations.size())
    {
        return;
    }

    // The name goes with the angle: both fields describe one row, and they are written back together.
    m_rotations[row].name = ui->lineEditName->text();
    m_rotations[row].angle = m_formulaAngle->formula();

    RefreshRow(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogSpread::ShowError(const QString &message)
{
    ui->labelStatus->setText(message);
    ui->labelStatus->setVisible(true);
}
