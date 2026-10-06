/************************************************************************
 **
 **  @file   odialogpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The dialog of the pleats: the value and the boxes of each of them, and the figure.
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

#include "odialogpleat.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QLineF>
#include <QMap>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "../../tools/drawTools/operation/oedgereader.h"
#include "../../tools/drawTools/operation/opleatsupport.h"
#include "../../visualization/line/operation/ovistoolpleat.h"
#include "../../visualization/visualization.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vmodifierkey.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vpatterndb/vcontainer.h"
#include "../widgets/formulainputwidget.h"
#include "../dialogtoolbox.h"
#include "ui_odialogpleat.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
constexpr int kPenIconWidth = 80;
constexpr int kPenIconHeight = 14;
constexpr int kColorIconSize = 20;
constexpr int kTableHeight = 90;

/** How parallel to a departure line an edge has to be for no cap to exist there — the same tolerance the rule
 *  itself uses to say that the two lines never meet. */
constexpr qreal kUnusableEdgeTolerance = 1e-7;

/** What a departure line that holds no value yet is given while the gesture runs: the caps are judged on the
 *  canvas, and a cap is a direction — no value, however small, changes the answer. */
constexpr qreal kNominalValue = 1.;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODialogPleat::ODialogPleat(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogPleat)
{
    ui->setupUi(this);

    // The table of the pleats opens the group and takes the whole width: it is what the rest of the window edits,
    // and the lines are picked on the canvas — it holds no geometry of its own, one line per departure line.
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Pleat"), tr("Value"), tr("Kind")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setMinimumHeight(kTableHeight);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ODialogPleat::DepartureSelected);
    ui->verticalLayoutPleat->insertWidget(0, m_table);

    auto *buttons = new QWidget(this);
    auto *buttonsLayout = new QHBoxLayout(buttons);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    m_add = new QPushButton(tr("Add a pleat"), buttons);
    m_remove = new QPushButton(tr("Remove the pleat"), buttons);
    buttonsLayout->addWidget(m_add);
    buttonsLayout->addWidget(m_remove);
    buttonsLayout->addStretch();
    connect(m_add, &QPushButton::clicked, this, &ODialogPleat::AddDeparture);
    connect(m_remove, &QPushButton::clicked, this, &ODialogPleat::RemoveDeparture);

    // The value comes after them, in the same block: what the field holds is the value of the line the table has
    // selected, and the window reads in that order.
    ui->verticalLayoutPleat->insertWidget(1, buttons);

    // The value the user types is the pleat as it will show once folded: the fabric it takes is twice that, and the
    // tool works that out. Nothing else is asked: the axis of the move is perpendicular to the departure line by
    // definition, and the value belongs to the line the table has selected.
    m_formulaValue = new FormulaInputWidget(this);
    m_formulaValue->setData(&this->data);
    m_formulaValue->setToolId(toolId);
    m_formulaValue->setLabelText(tr("Value of the finished pleat:"));
    m_formulaValue->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    m_formulaValue->setDialogTitle(tr("Value of the finished pleat"));
    m_formulaValue->setCheckZero(true);
    m_formulaValue->setCheckLessThanZero(true);
    connect(m_formulaValue, &FormulaInputWidget::valueChanged, this, &ODialogPleat::ValueChanged);

    InsertFormulaField(ui->formulaValuePlaceholder, m_formulaValue);
    ui->formulaValuePlaceholder = nullptr;

    // One fold, or two side by side, for the departure line the table has selected: a combo of the house, built here
    // like the others so it carries the same look and the same behaviour on every system.
    m_kind = MakeComboBox(this);
    m_kind->addItem(tr("Simple"), QVariant(static_cast<int>(OPleatKind::Simple)));
    m_kind->addItem(tr("Double"), QVariant(static_cast<int>(OPleatKind::Double)));
    ui->formLayoutPleat->insertRow(0, tr("Pleat:"), m_kind);

    const auto styles = [](QComboBox *combo) -> QMap<QString, QIcon>
    { return LineStylesPics(combo->palette().color(QPalette::Base), combo->palette().color(QPalette::Text)); };

    m_lineType = MakeComboBox(this);
    m_lineType->setMinimumWidth(kPenIconWidth);
    m_lineType->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_lineType->setIconSize(QSize(kPenIconWidth, kPenIconHeight));
    FillComboBoxTypeLine(m_lineType, styles(m_lineType), TypeLineLine);

    m_lineColor = MakeComboBox(this);
    m_lineColor->setMinimumWidth(kPenIconWidth);
    m_lineColor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_lineColor->setIconSize(QSize(kColorIconSize, kColorIconSize));
    InitColorCombo(m_lineColor);
    SetColorComboCurrentName(m_lineColor, ColorOrange);

    m_foldType = MakeComboBox(this);
    m_foldType->setMinimumWidth(kPenIconWidth);
    m_foldType->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_foldType->setIconSize(QSize(kPenIconWidth, kPenIconHeight));
    FillComboBoxTypeLine(m_foldType, styles(m_foldType), TypeLineDotLine);

    m_foldColor = MakeComboBox(this);
    m_foldColor->setMinimumWidth(kPenIconWidth);
    m_foldColor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_foldColor->setIconSize(QSize(kColorIconSize, kColorIconSize));
    InitColorCombo(m_foldColor);
    SetColorComboCurrentName(m_foldColor, ColorLightGreen);

    // The pens of the pleats — their lines, and the sides of their caps, which are drawn with them — belong to the
    // group that holds the pleats. The fold line has its own, in the group that holds the fold line alone.
    ui->formLayoutPleat->addRow(tr("Line type:"), m_lineType);
    ui->formLayoutPleat->addRow(tr("Line color:"), m_lineColor);
    ui->formLayoutDisplay->addRow(tr("Fold line type:"), m_foldType);
    ui->formLayoutDisplay->addRow(tr("Fold line color:"), m_foldColor);

    connect(ui->checkBoxCap1, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap2, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap1Sense, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap2Sense, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxNotch1, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxNotch2, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxFoldLine, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(m_kind, &QComboBox::currentIndexChanged, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);

    InitOkCancelApply(ui);

    ui->tabWidget->setCurrentIndex(0);
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide();
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);

    vis = new OVisToolPleat(&this->data);

    EnableDetails(false);
    RefreshCapLabels();
    RefreshFoldLineState();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogPleat::~ODialogPleat()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::Kind() const -> OPleatKind
{
    return (m_kind->currentData().toInt() == static_cast<int>(OPleatKind::Double)) ? OPleatKind::Double
                                                                                  : OPleatKind::Simple;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::Pleats() const -> int
{
    int count = 0;

    for (const OPleatDeparture &departure : m_departures)
    {
        count += (departure.kind == OPleatKind::Double) ? 2 : 1;
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::ShownDepartures() const -> QVector<OPleatDeparture>
{
    QVector<OPleatDeparture> departures = m_departures;

    // The editor holds the line it is showing: the geometry of a row comes from what the user typed in it, and the
    // table and the figure read the very same state.
    if (m_editorRow >= 0 && m_editorRow < departures.size())
    {
        OPleatDeparture &departure = departures[m_editorRow];
        departure.value = m_formulaValue->formula();
        departure.kind = Kind();
        departure.first.showCap = ui->checkBoxCap1->isChecked();
        departure.first.secondSense = ui->checkBoxCap1Sense->isChecked();
        departure.second.showCap = ui->checkBoxCap2->isChecked();
        departure.second.secondSense = ui->checkBoxCap2Sense->isChecked();
        departure.first.showNotch = ui->checkBoxNotch1->isChecked();
        departure.second.showNotch = ui->checkBoxNotch2->isChecked();
    }

    return departures;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetDepartures() const -> QVector<OPleatDeparture>
{
    return ShownDepartures();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::IsFoldLineShown() const -> bool
{
    return ui->checkBoxFoldLine->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetDisplay() const -> OPleatDisplay
{
    OPleatDisplay display = m_display;

    // The figure of a pleat is not optional: it always shows what the fold is made of, and only its fold lines can
    // be turned off.
    display.show = true;
    display.lineTypeLine = GetComboBoxCurrentData(m_lineType, TypeLineLine);
    display.lineColor = GetColorComboCurrentName(m_lineColor);
    display.foldTypeLine = GetComboBoxCurrentData(m_foldType, TypeLineDotLine);
    display.foldColor = GetColorComboCurrentName(m_foldColor);
    return display;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::SetPleat(const QVector<OPleatDeparture> &departures, bool foldLine)
{
    m_departures = departures;

    ui->checkBoxFoldLine->setChecked(foldLine);

    // An existing operation is complete: the dialog is opened to edit it, not to go through the gesture again. Its
    // objects carry the name of the copies they are, so only a new count of pleats asks for a new one.
    if (this->toolId != NULL_ID)
    {
        m_step = Step::Editing;
        m_sourcesNamed = true;
        m_namedPleats = Pleats();
    }

    m_editorRow = departures.isEmpty() ? -1 : 0;
    FillDepartures();
    RefreshCapLabels();
    RefreshFoldLineState();
    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::SetDisplay(const OPleatDisplay &display)
{
    m_display = display;
    m_figureLines = HeldLines(display); // what the operation draws: the state of the window is compared to it

    ChangeCurrentData(m_lineType, display.lineTypeLine);
    SetColorComboCurrentName(m_lineColor, display.lineColor);
    ChangeCurrentData(m_foldType, display.foldTypeLine);
    SetColorComboCurrentName(m_foldColor, display.foldColor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::HeldLines(const OPleatDisplay &display) -> FigureLines
{
    FigureLines lines;
    lines.places = static_cast<int>(display.lines.size());
    lines.foldLines = static_cast<int>(display.foldLines.size());

    for (const OPleatMarkDisplay &mark : display.marks)
    {
        if (mark.side1 != NULL_ID)
        {
            lines.markSides += 2; // both sides of the V, or neither of them
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::CurrentLines(const QVector<OPleatDeparture> &departures, bool foldLine) const -> FigureLines
{
    FigureLines lines;
    if (departures.isEmpty())
    {
        return lines;
    }

    const OPleatPlan plan = MakePlan(false);
    if (plan.failure != OPleatFailure::None)
    {
        return lines; // the rule refused the pleats: no figure is drawn, and nothing is held
    }

    lines.places = OPleatDisplay::LineCount(departures);
    lines.foldLines = foldLine ? plan.PleatCount() : 0;

    for (const OPleatMark &mark : plan.marks)
    {
        if (not mark.flat)
        {
            lines.markSides += 2; // an edge running along the move has no V to draw, and so no side
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::LinesChanged() const -> bool
{
    if (this->toolId == NULL_ID)
    {
        return false; // a pleat being created: nothing of it is in the pattern yet, and OK builds the whole of it
    }

    const FigureLines shown = CurrentLines(ShownDepartures(), ui->checkBoxFoldLine->isChecked());
    return shown.places != m_figureLines.places || shown.foldLines != m_figureLines.foldLines
           || shown.markSides != m_figureLines.markSides;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::CheckState()
{
    DialogOperationTool::CheckState();

    if (bApply == nullptr || not bApply->isEnabled() || not LinesChanged())
    {
        return;
    }

    // A cap turned off or on, the fold lines, a pleat added: the figure would draw one line more or one less, and
    // only a full parse builds a line — the very parse that would rebuild the operation and reject this dialog. The
    // change therefore waits for OK, as every structural change of an operation does. What the figure only draws as
    // points and curves — the notch of an end, and the two halves it cuts its edge into — is applied in place: the
    // tool gives those objects their item on the parse the change asks for.
    bApply->setEnabled(false);
    bApply->setToolTip(tr("Adding or removing objects can't be applied while the dialog stays open. "
                          "Use OK to confirm."));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::ValueInPixels(const QString &formula, const VContainer *data) -> qreal
{
    if (formula.isEmpty())
    {
        return 0.;
    }

    // A formula that does not hold answers nothing: the dialog draws no fold at all while the user is typing one, and
    // never a fold built on a value the tool would refuse.
    const qreal read = Visualization::FindValFromUser(formula, data->DataVariables());
    return (read > 0. && !qIsNaN(read) && !qIsInf(read)) ? VAbstractValApplication::VApp()->toPixel(read) : 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::Values(const QVector<OPleatDeparture> &departures, bool nominal) const -> QVector<qreal>
{
    QVector<qreal> values;
    values.reserve(departures.size());

    for (const OPleatDeparture &departure : departures)
    {
        qreal value = ValueInPixels(departure.value, &data);
        if (nominal && value <= 0.)
        {
            value = kNominalValue;
        }
        values.append(value);
    }

    return values;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::MakePlan(bool nominal) const -> OPleatPlan
{
    const QVector<OPleatDeparture> departures = ShownDepartures();
    return OPleatRule::Compose(departures, Values(departures, nominal), GetSourceObjects(), &data);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::FailureText(const OPleatPlan &plan) const -> QString
{
    switch (plan.failure)
    {
        case OPleatFailure::None:
            return {};
        case OPleatFailure::DegeneratePrise:
            return tr("You clicked the same point twice.");
        case OPleatFailure::DegenerateValue:
            return tr("Put a pleat value greater than zero.");
        case OPleatFailure::NoSide:
            return tr("Every object you selected must be on the same side of the departure lines.");
        case OPleatFailure::UnresolvedEdge:
            return tr("The edge was not recognized at one end of a departure line.");
        case OPleatFailure::EdgeAlongFold:
            return tr("This edge is parallel to the fold line: no cap is possible here.");
        case OPleatFailure::NotchOffEdge:
            return tr("The notch of a pleat falls past the end of the edge it should mark.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshFoldLineState()
{
    // The fold line of a pleat joins its two caps: with no pleat having both of them, the box has nothing to mean. It
    // keeps its state, so ticking a cap back brings the line back with it.
    const QVector<OPleatDeparture> departures = ShownDepartures();
    const bool any = std::any_of(departures.cbegin(), departures.cend(),
                                 [](const OPleatDeparture &departure)
                                 { return departure.first.showCap && departure.second.showCap; });
    ui->checkBoxFoldLine->setEnabled(any);
    RefreshNotchState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshNotchState()
{
    // A notch marks the edge the cap follows: with no cap at that end there is no edge to follow, and the box has
    // nothing to mean. It keeps its state, exactly as the box of the fold line does, so ticking the cap back brings
    // the notch back with it.
    ui->checkBoxNotch1->setEnabled(ui->checkBoxCap1->isEnabled() && ui->checkBoxCap1->isChecked());
    ui->checkBoxNotch2->setEnabled(ui->checkBoxCap2->isEnabled() && ui->checkBoxCap2->isChecked());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshFigure()
{
    if (vis.isNull())
    {
        // A full parse — the one an option that changes the objects of the pleats asks for — clears the scene and takes
        // this preview with it, while the dialog stays up. It is built again from the dialog's own state, so what the
        // user is changing keeps showing.
        vis = new OVisToolPleat(&this->data);
    }

    auto *operation = qobject_cast<OVisToolPleat *>(vis);
    if (operation == nullptr)
    {
        return;
    }

    // A pleat the dialog did not create yet has nothing in the pattern: the preview is then the only thing that shows
    // what the folds will do. As soon as the operation exists — the dialog reopened on it, or its own Apply having
    // built it — the pattern draws all of that, and the preview keeps the figure (see OVisToolPleat::SetInPattern).
    operation->SetInPattern(this->toolId != NULL_ID);
    operation->SetObjects(SourceToObjects(GetSourceObjects()));
    operation->SetPreviewLines(PreviewLines());
    operation->SetPleat(ShownDepartures(), m_pending, ui->checkBoxFoldLine->isChecked(), m_editorRow);

    if (not VAbstractValApplication::VApp()->getCurrentScene()->items().contains(operation))
    {
        operation->VisualMode(NULL_ID);
    }
    else
    {
        operation->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::FigureOptionChanged()
{
    const int row = CurrentRow();
    if (row >= 0 && row < m_departures.size())
    {
        // What the editor shows is what the row holds: a box that is ticked is the row's, and the table says so.
        m_departures[row].kind = Kind();
        m_departures[row].first.showCap = ui->checkBoxCap1->isChecked();
        m_departures[row].first.secondSense = ui->checkBoxCap1Sense->isChecked();
        m_departures[row].second.showCap = ui->checkBoxCap2->isChecked();
        m_departures[row].second.secondSense = ui->checkBoxCap2Sense->isChecked();
        m_departures[row].first.showNotch = ui->checkBoxNotch1->isChecked();
        m_departures[row].second.showNotch = ui->checkBoxNotch2->isChecked();
        RefreshRow(row);
    }

    RefreshFoldLineState();
    RefreshFigure();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ValueChanged(bool valid)
{
    Q_UNUSED(valid)

    const int row = CurrentRow();
    if (row >= 0 && row < m_departures.size())
    {
        m_departures[row].value = m_formulaValue->formula();
        RefreshRow(row);
    }

    RefreshFigure();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::IsValid() const -> bool
{
    if (ShownDepartures().isEmpty())
    {
        ShowError(tr("Click the two points of the first departure line."));
        return false;
    }

    const QVector<OPleatDeparture> departures = ShownDepartures();
    for (const qreal value : Values(departures, false))
    {
        if (value <= 0.)
        {
            ShowError(tr("Put a pleat value greater than zero."));
            return false;
        }
    }

    const OPleatPlan plan = MakePlan(false);
    if (plan.failure != OPleatFailure::None)
    {
        ShowError(FailureText(plan));
        return false;
    }

    if (not IsFlagGroupName())
    {
        ShowError(tr("Invalid layer name"));
        return false;
    }

    ui->labelStatus->setText(tr("Ready"));
    ui->labelStatus->setVisible(false);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ShowError(const QString &message) const
{
    ui->labelStatus->setText(message);
    ui->labelStatus->setVisible(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::EdgePrompt(int rank) const -> QString
{
    return rank == 0 ? tr("The edge is not recognized at the cap of point 1 (green): click a point of the "
                          "piece's outline, right next to it")
                     : tr("The edge is not recognized at the cap of point 2 (red): click a point of the piece's "
                          "outline, right next to it");
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::DiscardUnusableEdges(int departure)
{
    // What the tool read, and what no cap can be built from, is not an answer. The dialog has no way of its own to
    // name an edge: it would open on a plan its OK refuses, with nothing in the window able to fix it. The point is
    // asked for on the canvas instead, and the dialog opens only once every cap it needs can be built.
    if (departure < 0 || departure >= m_departures.size())
    {
        return;
    }

    OPleatDeparture &line = m_departures[departure];

    const QPointF fold = line.second.position - line.first.position;
    const qreal foldLength = QLineF(QPointF(), fold).length();

    if (foldLength > 0.)
    {
        // An edge that runs along the departure line meets it nowhere: the two lines are parallel, and no cap
        // exists. It is the line of the fold itself — or a construction line drawn through both points of the
        // departure line — and never the edge of the piece.
        const QPointF alongFold = fold / foldLength;
        for (int rank = 0; rank < 2; ++rank)
        {
            OPleatEnd &end = (rank == 1) ? line.second : line.first;
            if (not end.showCap || end.edgePointId != NULL_ID)
            {
                continue; // no cap there, or the user has already said which point carries its edge
            }

            const qreal length = QLineF(QPointF(), end.edgeDirection).length();
            if (length <= 0.)
            {
                continue; // nothing was read: the point is asked for anyway
            }

            const QPointF edge = end.edgeDirection / length;
            if (qAbs(edge.x() * alongFold.x() + edge.y() * alongFold.y()) > 1. - kUnusableEdgeTolerance)
            {
                end.edgeDirection = QPointF();
            }
        }
    }

    if (MakePlan(true).failure != OPleatFailure::EdgeAlongFold)
    {
        return; // the plan complains about something else, or about nothing
    }

    // The plan still refuses an edge for a reason of its own: nothing the tool read may be trusted, and the user is
    // asked for both ends it concerns.
    for (int rank = 0; rank < 2; ++rank)
    {
        OPleatEnd &end = (rank == 1) ? line.second : line.first;
        if (end.showCap && end.edgePointId == NULL_ID && QLineF(QPointF(), end.edgeDirection).length() > 0.)
        {
            end.edgeDirection = QPointF();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::AskForEdge() -> bool
{
    for (int departure = 0; departure < m_departures.size(); ++departure)
    {
        DiscardUnusableEdges(departure);
    }

    for (int departure = 0; departure < m_departures.size(); ++departure)
    {
        const OPleatDeparture &line = m_departures.at(departure);

        for (int rank = 0; rank < 2; ++rank)
        {
            const OPleatEnd &end = (rank == 0) ? line.first : line.second;
            if (not end.showCap || end.edgePointId != NULL_ID)
            {
                continue; // no cap there, or the user has already said which point carries its edge
            }

            if (QLineF(QPointF(), end.edgeDirection).length() > 0.)
            {
                continue; // the tool read the edge from the point's own tool: nothing to ask
            }

            m_edgeDeparture = departure;
            m_edgeRank = rank;
            m_step = Step::Edge;
            emit ToolTip(EdgePrompt(rank));
            return false;
        }
    }

    return true; // every cap has an edge: the caller goes on, or opens the dialog
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshCapLabels()
{
    // A dot of the colour the point takes on the canvas: green for point 1 of a departure line, red for point 2. The
    // colour is the name — a point name would say nothing to a modéliste who works with the names turned off.
    const auto dot = [](VColorRole role) -> QString
    {
        return QStringLiteral("<span style='font-size:15pt; color:%1'>&#9679;</span>")
            .arg(VSceneStylesheet::Color(role).name());
    };

    ui->labelCap1->setText(QStringLiteral("%1 %2").arg(dot(VColorRole::VisSupportColor2), tr("Cap 1:")));
    ui->labelCap2->setText(QStringLiteral("%1 %2").arg(dot(VColorRole::VisSupportColor4), tr("Cap 2:")));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::PointPrompt() const -> QString
{
    // Point 1 turns green and point 2 red: that colour is what names them everywhere else — in the dialog, in the dock
    // and on the canvas — so the prompt says it, and never a point name. The pleat being built is named by its rank,
    // which is the very number the table of the dialog will hold it under.
    const int pleat = static_cast<int>(m_departures.size()) + 1;

    if (m_pointRank == 0)
    {
        return m_departures.isEmpty()
                   ? tr("Click point 1 of the departure line of pleat 1 (it turns green)")
                   : tr("Click point 1 of the departure line of pleat %1 (it turns green), or %2 to finish")
                         .arg(pleat)
                         .arg(VModifierKey::EnterKey());
    }

    return tr("Click point 2 of the departure line of pleat %1 (it turns red)").arg(pleat);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::NameSources(bool force)
{
    const int pleats = qMax(1, Pleats());

    if (not force && m_sourcesNamed && m_namedPleats == pleats)
    {
        return; // the objects already carry the name of the pleats they are about to become
    }

    const QString suffix = "__m"_L1 + QString::number(pleats);

    for (auto &item : SourceObjects())
    {
        if (item.id == NULL_ID)
        {
            continue;
        }

        try
        {
            const QSharedPointer<VGObject> object = data.GetGObject(item.id);
            const QString base = (object->getType() == GOType::Point)
                                     ? object->name()
                                     : data.GeometricObject<VAbstractCurve>(item.id)->HeadlessName();

            const QString name = base + suffix;
            item.name = data.IsUnique(name) ? name
                                            : name + '_'_L1 + QString::number(data.UniqueDuplicate(name));
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }
    }

    m_sourcesNamed = true;
    m_namedPleats = pleats;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::CurrentRow() const -> int
{
    if (m_table == nullptr || m_step != Step::Editing)
    {
        return -1;
    }

    const int row = m_table->currentRow();
    return (row >= 0 && row < m_departures.size()) ? row : -1;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshRow(int row)
{
    if (row < 0 || row >= m_departures.size() || row >= m_table->rowCount())
    {
        return;
    }

    const OPleatDeparture &departure = m_departures.at(row);

    const auto cell = [this, row](int column, const QString &text)
    {
        auto *item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, column, item);
    };

    cell(0, tr("Pleat %1").arg(row + 1));
    cell(1, departure.value.isEmpty() ? QStringLiteral("—") : departure.value);
    cell(2, departure.kind == OPleatKind::Double ? tr("Double") : tr("Simple"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ShowDepartureDetails(int row)
{
    EnableDetails(row >= 0);

    if (row < 0 || row >= m_departures.size())
    {
        return;
    }

    const OPleatDeparture &departure = m_departures.at(row);

    // The editor is loaded, not edited: nothing of what it says must be written back while it is being filled.
    const QSignalBlocker blockFormula(m_formulaValue);
    const QSignalBlocker blockKind(m_kind);
    const QSignalBlocker blockCap1(ui->checkBoxCap1);
    const QSignalBlocker blockCap1Sense(ui->checkBoxCap1Sense);
    const QSignalBlocker blockCap2(ui->checkBoxCap2);
    const QSignalBlocker blockCap2Sense(ui->checkBoxCap2Sense);
    const QSignalBlocker blockNotch1(ui->checkBoxNotch1);
    const QSignalBlocker blockNotch2(ui->checkBoxNotch2);

    m_formulaValue->setFormula(departure.value);
    ChangeCurrentData(m_kind, static_cast<int>(departure.kind));
    ui->checkBoxCap1->setChecked(departure.first.showCap);
    ui->checkBoxCap1Sense->setChecked(departure.first.secondSense);
    ui->checkBoxCap2->setChecked(departure.second.showCap);
    ui->checkBoxCap2Sense->setChecked(departure.second.secondSense);
    ui->checkBoxNotch1->setChecked(departure.first.showNotch);
    ui->checkBoxNotch2->setChecked(departure.second.showNotch);

    RefreshNotchState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::EnableDetails(bool enabled)
{
    m_kind->setEnabled(enabled);
    m_formulaValue->setEnabled(enabled);
    ui->checkBoxCap1->setEnabled(enabled);
    ui->checkBoxCap1Sense->setEnabled(enabled);
    ui->checkBoxCap2->setEnabled(enabled);
    ui->checkBoxCap2Sense->setEnabled(enabled);
    m_remove->setEnabled(enabled && m_departures.size() > 1);

    RefreshNotchState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::FillDepartures()
{
    const int rows = static_cast<int>(m_departures.size());

    {
        const QSignalBlocker blocker(m_table);
        m_table->setRowCount(rows);

        for (int row = 0; row < rows; ++row)
        {
            RefreshRow(row);
        }

        if (rows > 0)
        {
            const int select = (m_editorRow >= 0 && m_editorRow < rows) ? m_editorRow : 0;
            m_table->selectRow(select);
            m_editorRow = select;
        }
        else
        {
            m_editorRow = -1;
        }
    }

    ShowDepartureDetails(m_editorRow);
    m_add->setEnabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::DepartureSelected()
{
    const int row = CurrentRow();
    if (row == m_editorRow)
    {
        return;
    }

    m_editorRow = row;
    ShowDepartureDetails(row);
    RefreshFoldLineState();
    RefreshFigure(); // the canvas follows the table: the two ends drawn bright are the ones of the line being edited
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::AddDeparture()
{
    // The line is picked on the canvas: the dialog steps aside, and comes back once the pair is complete — the very
    // gesture that built the others. The parts of the window that belong to a line are put away meanwhile.
    m_editorRow = -1;
    m_addingDeparture = true;
    m_pending = OPleatEnd{};
    m_pointRank = 0;
    m_step = Step::Points;

    hide();
    emit ToolTip(PointPrompt());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RemoveDeparture()
{
    const int row = CurrentRow();
    if (row < 0 || row >= m_departures.size() || m_departures.size() <= 1)
    {
        return;
    }

    m_departures.removeAt(row);
    m_editorRow = qMin(row, static_cast<int>(m_departures.size()) - 1);
    FillDepartures();
    RefreshFoldLineState();
    RefreshFigure();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::GoToEditing(int row)
{
    m_step = Step::Editing;
    m_addingDeparture = false;
    m_pending = OPleatEnd{};
    m_editorRow = (row >= 0 && row < m_departures.size()) ? row : 0;

    FillDepartures();

    setModal(true);
    emit ToolTip(QString());
    RefreshFigure();
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ShowDialog(bool click)
{
    if (m_step == Step::Selecting && not click)
    {
        if (GetSourceObjects().isEmpty())
        {
            emit ToolTip(tr("Select the objects of the contour, then %1").arg(VModifierKey::EnterKey()));
            return;
        }

        // The objects are about to become copies: they take the name of the copy they will be, now that the set is
        // closed and before the tool reads it.
        NameSources(true);

        // And now the departure lines: their points, two by two, in the order that gives them their colour.
        m_step = Step::Points;
        m_pointRank = 0;
        m_pending = OPleatEnd{};
        emit ToolTip(PointPrompt());
        return;
    }

    if (m_step == Step::Points && not click)
    {
        if (m_pointRank == 0 && not m_departures.isEmpty())
        {
            // Enter closes the series: every line is drawn, and every cap it needs can be built — the dialog only
            // opens on a pleat the tool will be able to build.
            if (AskForEdge())
            {
                GoToEditing(m_addingDeparture ? static_cast<int>(m_departures.size()) - 1 : 0);
            }
            return;
        }

        emit ToolTip(PointPrompt()); // Enter says nothing more: the two points are clicks, not a confirmation
        return;
    }

    if (m_step == Step::Edge && not click)
    {
        // Enter says nothing here: a cap without an edge cannot be built, the dialog would open on a plan its OK
        // refuses, and nothing in the window can name an edge. The status bar keeps asking for the point.
        emit ToolTip(EdgePrompt(m_edgeRank));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ChosenObject(quint32 id, const SceneObject &type)
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

        RefreshFigure();
        CheckState();
        return;
    }

    if (m_step == Step::Points)
    {
        if (type != SceneObject::Point)
        {
            emit ToolTip(tr("A departure line is made of two points"));
            return;
        }

        // The points are part of what travels — they are picked among the objects, as the chain of a
        // slash-and-spread is — so their copies exist and the folds have somewhere to land. A point that was not
        // selected has no copy, and the pleats could not be built on it.
        const bool isSource = std::any_of(GetSourceObjects().cbegin(), GetSourceObjects().cend(),
                                          [id](const SourceItem &item) { return item.id == id; });
        if (not isSource)
        {
            emit ToolTip(tr("This point is not one of the objects you selected: click a point of the contour"));
            return;
        }

        OPleatEnd end;
        end.pointId = id;
        OPleatSupport::Resolve(end, &data);

        if (m_pointRank == 0)
        {
            m_pending = end;
            m_pointRank = 1;
            RefreshCapLabels();
            emit ToolTip(PointPrompt());
        }
        else
        {
            if (end.pointId == m_pending.pointId)
            {
                emit ToolTip(FailureText(OPleatPlan{.failure = OPleatFailure::DegeneratePrise}));
                return;
            }

            OPleatDeparture departure;
            departure.first = m_pending;
            departure.second = end;
            departure.value = m_formulaValue->formula();
            departure.kind = Kind();

            // Every line is folded along its own normal, so two pleats of one operation may perfectly well be taken
            // along two different axes: nothing has to be checked here, and the new line is taken as it is.
            m_departures.append(departure);
            m_pending = OPleatEnd{};
            m_pointRank = 0;
            RefreshCapLabels();

            // The line is complete: its caps are asked for, and the gesture then goes on with the next departure
            // line — that is what "several pleats in one pass" means. Enter is what opens the dialog.
            if (AskForEdge())
            {
                emit ToolTip(PointPrompt());
            }
        }

        RefreshFigure();
        CheckState();
        return;
    }

    if (m_step == Step::Edge)
    {
        if (type != SceneObject::Point)
        {
            emit ToolTip(tr("Click a point of the outline, right next to it"));
            return;
        }

        if (m_edgeDeparture < 0 || m_edgeDeparture >= m_departures.size())
        {
            return;
        }

        OPleatDeparture &line = m_departures[m_edgeDeparture];
        OPleatEnd end = (m_edgeRank == 1) ? line.second : line.first;
        end.edgePointId = id;
        OPleatSupport::Resolve(end, &data);

        if (m_edgeRank == 1)
        {
            line.second = end;
        }
        else
        {
            line.first = end;
        }

        // A point whose line builds no cap is refused here, on the canvas, rather than in the dialog: the status
        // bar asks for another one and the step does not move. The dialog must never open on a cap it cannot build.
        const OPleatPlan plan = MakePlan(true);
        if (plan.failure == OPleatFailure::EdgeAlongFold)
        {
            OPleatEnd &refused = (m_edgeRank == 1) ? line.second : line.first;
            refused.edgePointId = NULL_ID;
            OPleatSupport::Resolve(refused, &data); // back to whatever the tool reads by itself
            emit ToolTip(QStringLiteral("%1 %2").arg(FailureText(plan),
                                                     tr("Click a point of the outline, right next to it")));
            RefreshFigure(); // the preview shows what the tool reads now, not the point that was refused
            return;
        }

        if (AskForEdge())
        {
            emit ToolTip(PointPrompt());
            RefreshFigure();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::OnSourceObjectsSet()
{
    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ShowVisualization()
{
    AddVisualization<OVisToolPleat>();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::SaveData()
{
    NameSources(false); // a new count of pleats renames the copies, the same count keeps the names
    RefreshFigure();
    CheckState();

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
void ODialogPleat::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        m_formulaValue->setLabelText(tr("Value of the finished pleat:"));
        m_table->setHorizontalHeaderLabels({tr("Pleat"), tr("Value"), tr("Kind")});
        m_add->setText(tr("Add a pleat"));
        m_remove->setText(tr("Remove the pleat"));
        for (int row = 0; row < m_departures.size(); ++row)
        {
            RefreshRow(row);
        }
        RefreshCapLabels();
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}
