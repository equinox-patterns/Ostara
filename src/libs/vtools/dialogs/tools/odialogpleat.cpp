/************************************************************************
 **
 **  @file   odialogpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The dialog of a pleat: the prise, the objects that travel, the value, and what is drawn.
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
#include <QLineEdit>
#include <QLineF>
#include <QMap>
#include <QPalette>
#include <QPlainTextEdit>
#include <QTabBar>

#include "../../tools/drawTools/operation/opleatsupport.h"
#include "../../visualization/line/operation/ovistoolpleat.h"
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
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODialogPleat::ODialogPleat(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogPleat)
{
    ui->setupUi(this);

    // The value the user types is the pleat as it will show once folded: the fabric it takes is twice that, and the
    // tool works that out. Nothing else is asked: the axis of the move is perpendicular to the prise by definition.
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

    // One fold, or two side by side: a combo of the house, built here like the others so it carries the same look
    // and the same behaviour on every system. It opens the group, before the two caps it decides the meaning of.
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

    // The pens of the pleat itself — its lines, and the sides of its caps, which are drawn with them — belong to the
    // group that holds the pleat. The fold line has its own, in the group that holds the fold line alone.
    ui->formLayoutPleat->addRow(tr("Line type:"), m_lineType);
    ui->formLayoutPleat->addRow(tr("Line color:"), m_lineColor);
    ui->formLayoutDisplay->addRow(tr("Fold line type:"), m_foldType);
    ui->formLayoutDisplay->addRow(tr("Fold line color:"), m_foldColor);

    connect(ui->checkBoxCap1, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap2, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap1Sense, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
    connect(ui->checkBoxCap2Sense, &QCheckBox::toggled, this, &ODialogPleat::FigureOptionChanged);
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

    RefreshCapLabels();
    RefreshFoldLineState();
}

//---------------------------------------------------------------------------------------------------------------------
ODialogPleat::~ODialogPleat()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::ShownEnd(int rank) const -> OPleatEnd
{
    OPleatEnd end = (rank == 1) ? m_second : m_first;
    end.showCap = (rank == 1) ? ui->checkBoxCap2->isChecked() : ui->checkBoxCap1->isChecked();
    end.secondSense = (rank == 1) ? ui->checkBoxCap2Sense->isChecked() : ui->checkBoxCap1Sense->isChecked();
    return end;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetFirst() const -> OPleatEnd
{
    return ShownEnd(0);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetSecond() const -> OPleatEnd
{
    return ShownEnd(1);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetValue() const -> QString
{
    return m_formulaValue->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::GetKind() const -> OPleatKind
{
    return (m_kind->currentData().toInt() == static_cast<int>(OPleatKind::Double)) ? OPleatKind::Double
                                                                                  : OPleatKind::Simple;
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

    // The figure of a pleat is not optional: it always shows what the fold is made of, and only its fold line can
    // be turned off.
    display.show = true;
    display.lineTypeLine = GetComboBoxCurrentData(m_lineType, TypeLineLine);
    display.lineColor = GetColorComboCurrentName(m_lineColor);
    display.foldTypeLine = GetComboBoxCurrentData(m_foldType, TypeLineDotLine);
    display.foldColor = GetColorComboCurrentName(m_foldColor);
    return display;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::SetPleat(const OPleatEnd &first, const OPleatEnd &second, const QString &value, OPleatKind kind,
                            bool foldLine)
{
    m_first = first;
    m_second = second;

    m_formulaValue->setFormula(value);
    ChangeCurrentData(m_kind, static_cast<int>(kind));
    ui->checkBoxFoldLine->setChecked(foldLine);
    ui->checkBoxCap1->setChecked(first.showCap);
    ui->checkBoxCap1Sense->setChecked(first.secondSense);
    ui->checkBoxCap2->setChecked(second.showCap);
    ui->checkBoxCap2Sense->setChecked(second.secondSense);

    // An existing pleat is complete: the dialog is opened to edit it, not to go through the gesture again. Its
    // objects carry the name of the copies they are, so only a change of kind asks for a new one.
    if (toolId != NULL_ID)
    {
        m_step = Step::Editing;
        m_sourcesNamed = true;
        m_namedKind = kind;
    }

    RefreshCapLabels();
    RefreshFoldLineState();
    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::SetDisplay(const OPleatDisplay &display)
{
    m_display = display;

    ChangeCurrentData(m_lineType, display.lineTypeLine);
    SetColorComboCurrentName(m_lineColor, display.lineColor);
    ChangeCurrentData(m_foldType, display.foldTypeLine);
    SetColorComboCurrentName(m_foldColor, display.foldColor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::PleatValue() const -> qreal
{
    if (m_formulaValue == nullptr || m_formulaValue->isValid() == false)
    {
        return 0.;
    }

    const qreal value = m_formulaValue->value();
    return value > 0. ? VAbstractValApplication::VApp()->toPixel(value) : 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::MakePlan() const -> OPleatPlan
{
    const OPleatEnd first = ShownEnd(0);
    const OPleatEnd second = ShownEnd(1);

    const QVector<SourceItem> source = GetSourceObjects();
    return OPleatRule::Compose(first, second, PleatValue(), GetKind(),
                               OPleatRule::SideOf(source, first.pointId, second.pointId, &data));
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
            return tr("Every object you selected must be on the same side of the departure line.");
        case OPleatFailure::UnresolvedEdge:
            return tr("The edge was not recognized at one end of the departure line.");
        case OPleatFailure::EdgeAlongFold:
            return tr("This edge is parallel to the fold line: no cap is possible here.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshFoldLineState()
{
    // The fold line joins the two caps: without both of them there is nothing to join, and the box has nothing to
    // mean. It keeps its state, so ticking the cap back brings the line back with it.
    const bool bothCaps = ui->checkBoxCap1->isChecked() && ui->checkBoxCap2->isChecked();
    ui->checkBoxFoldLine->setEnabled(bothCaps);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshFigure()
{
    if (vis.isNull())
    {
        // A full parse — the one an option that changes the objects of the pleat asks for — clears the scene and takes
        // this preview with it, while the dialog stays up. It is built again from the dialog's own state, so what the
        // user is changing keeps showing.
        vis = new OVisToolPleat(&this->data);
    }

    auto *operation = qobject_cast<OVisToolPleat *>(vis);
    if (operation == nullptr)
    {
        return;
    }

    operation->SetObjects(SourceToObjects(GetSourceObjects()));
    operation->SetPreviewLines(PreviewLines());
    const bool bothCaps = ui->checkBoxCap1->isChecked() && ui->checkBoxCap2->isChecked();
    operation->SetPleat(ShownEnd(0), ShownEnd(1), m_formulaValue->formula(), GetKind(),
                        ui->checkBoxFoldLine->isChecked() && bothCaps);

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
    RefreshFoldLineState();
    RefreshFigure();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::ValueChanged(bool valid)
{
    Q_UNUSED(valid)
    RefreshFigure();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::IsValid() const -> bool
{
    if (m_first.pointId == NULL_ID || m_second.pointId == NULL_ID)
    {
        ShowError(tr("Click the two points of the departure line."));
        return false;
    }

    if (PleatValue() <= 0.)
    {
        ShowError(tr("Put a pleat value greater than zero."));
        return false;
    }

    const OPleatPlan plan = MakePlan();
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
void ODialogPleat::AskForEdge()
{
    for (int rank = 0; rank < 2; ++rank)
    {
        const OPleatEnd end = (rank == 1) ? m_second : m_first;
        if (not end.showCap || end.edgePointId != NULL_ID)
        {
            continue; // no cap there, or the user has already said which point carries its edge
        }

        if (QLineF(QPointF(), end.edgeDirection).length() > 0.)
        {
            continue; // the tool read the edge from the point's own tool: nothing to ask
        }

        m_edgeRank = rank;
        m_step = Step::Edge;
        emit ToolTip(rank == 0
                         ? tr("The edge is not recognized at the cap of point 1 (green): click a point of the "
                              "piece's outline, right next to it")
                         : tr("The edge is not recognized at the cap of point 2 (red): click a point of the piece's "
                              "outline, right next to it"));
        return;
    }

    GoToEditing();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::RefreshCapLabels()
{
    // A dot of the colour the point takes on the canvas: green for point 1 of the departure line, red for point 2.
    // The colour is the name — a point name would say nothing to a modéliste who works with the names turned off.
    const auto dot = [](VColorRole role) -> QString
    {
        return QStringLiteral("<span style='font-size:15pt; color:%1'>&#9679;</span>")
            .arg(VSceneStylesheet::Color(role).name());
    };

    ui->labelCap1->setText(QStringLiteral("%1 %2").arg(dot(VColorRole::VisSupportColor2), tr("Cap 1:")));
    ui->labelCap2->setText(QStringLiteral("%1 %2").arg(dot(VColorRole::VisSupportColor4), tr("Cap 2:")));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::NameSources(bool force)
{
    const OPleatKind kind = GetKind();

    if (not force && m_sourcesNamed && m_namedKind == kind)
    {
        return; // the objects already carry the name of the pleat they are about to become
    }

    const QString suffix = (kind == OPleatKind::Double) ? "__m2"_L1 : "__m1"_L1;

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
    m_namedKind = kind;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogPleat::GoToEditing()
{
    m_step = Step::Editing;

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

        // And now the departure line: its two points, in the order that gives them their colour.
        m_step = Step::Points;
        m_pointRank = 0;
        emit ToolTip(PointPrompt());
        return;
    }

    if (m_step == Step::Points && not click)
    {
        emit ToolTip(PointPrompt()); // Enter says nothing more: the two points are clicks, not a confirmation
        return;
    }

    if (m_step == Step::Edge && not click)
    {
        // Enter while an edge is missing: the dialog opens all the same, and says what is missing.
        GoToEditing();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::PointPrompt() const -> QString
{
    // Point 1 turns green and point 2 red: that colour is what names them everywhere else — in the dialog, in the dock
    // and on the canvas — so the prompt says it, and never a point name.
    return m_pointRank == 0 ? tr("Click point 1 of the pleat's departure line (it turns green)")
                            : tr("Click point 2 of the pleat's departure line (it turns red)");
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
            emit ToolTip(tr("The departure line is made of two points"));
            return;
        }

        // The two points are part of what travels — they are picked among the objects, as the chain of a
        // slash-and-spread is — so their copies exist and the fold has somewhere to land. A point that was not
        // selected has no copy, and the pleat could not be built on it.
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
            m_first = end;
            m_pointRank = 1;
            RefreshCapLabels();
            emit ToolTip(PointPrompt());
        }
        else
        {
            m_second = end;
            RefreshCapLabels();
            AskForEdge(); // no edge to ask for: the dialog opens, and the figure is drawn from what it knows
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

        OPleatEnd end = (m_edgeRank == 1) ? m_second : m_first;
        end.edgePointId = id;
        OPleatSupport::Resolve(end, &data);
        (m_edgeRank == 1) ? m_second = end : m_first = end;

        AskForEdge();
        RefreshFigure();
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
    NameSources(false); // a change of kind renames the copies, a simple pleat keeps the name it has
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
        RefreshCapLabels();
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogPleat::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}
