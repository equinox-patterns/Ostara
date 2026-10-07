/************************************************************************
 **
 **  @file   odialogtransfert.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  The dialog of the transfer: the host part and its layer, and the two clicks on the canvas that
 **          anchor the portion — before it for X, after it for A.
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

#include "odialogtransfert.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDomElement>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSharedPointer>
#include <QStringList>
#include <QTabBar>
#include <QVariant>
#include <QtAlgorithms>

#include "../../tools/toolsdef.h"
#include "../../visualization/line/operation/ovistooltransfert.h"
#include "../../visualization/visualization.h"
#include "../dialogtoolbox.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "ui_odialogtransfert.h"

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
ODialogTransfert::ODialogTransfert(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogTransfert)
{
    ui->setupUi(this);

    // Remember where the gesture started. The active part moves to the host part in the middle of the creation,
    // and that move is the window's, not this dialog's — so the way back has to be known here, where the step
    // that made the move is.
    if (not IsEditing())
    {
        m_sourcePart = m_doc->PatternBlockMapper()->GetActive();
    }

    // The objects of a transfer are the ones picked on the canvas, and nothing else: there is no combo to add
    // one, and no list to remove one from (the .ui keeps it hidden).
    ApplyComboBoxView(ui->comboBoxPiece);
    ApplyComboBoxView(ui->comboBoxLayer);
    MakeFilterableComboBox(ui->comboBoxLayer);
    // A layer is typed only to create one: typing a name must not append a row to the list of the existing
    // layers, or the same layer would be offered twice.
    ui->comboBoxLayer->setInsertPolicy(QComboBox::NoInsert);


    // The properties of a source object are not this tool's business, and the portion is displayed, not
    // edited: the whole list of the template — its contents, its rename button and its removal button — stays
    // hidden in the .ui, and nothing here brings it back. What is picked is shown on the canvas.
    connect(ui->comboBoxPiece, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODialogTransfert::HostPartChanged);

    // An existing transfer already knows its two points: its dialog is a view of them, and neither the part nor
    // the layer can be moved under the copies that are already written.
    if (IsEditing())
    {
        ui->comboBoxPiece->setEnabled(false);
        ui->comboBoxLayer->setEnabled(false);
    }

    InitOkCancel(ui);

    // The portion is shown while it is picked, like every operation: the picked objects are lit, and nothing
    // else is drawn — the anchors of the transfer are not known yet.
    vis = new OVisToolTransfert(&this->data);
    ShowVisualization();

    ui->tabWidget->setCurrentIndex(0);

    // One page only: the layer and the notes, next to the list of what is transferred.
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide();
    }

    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
ODialogTransfert::~ODialogTransfert()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, and the values the dialog shows follow.
        vis = new OVisToolTransfert(&this->data);
    }

    AddVisualization<OVisToolTransfert>();
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::OnSourceObjectsSet()
{
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::RefreshPreview()
{
    if (vis.isNull())
    {
        return;
    }

    auto *visual = qobject_cast<OVisToolTransfert *>(vis);
    SCASSERT(visual != nullptr)

    visual->SetObjects(SourceToObjects(SourceObjects()));
    visual->SetPreviewLines(PreviewLines());
    visual->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::Say(const QString &step)
{
    if (Widgets().labelStatus != nullptr)
    {
        Widgets().labelStatus->setText(step);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::IsEditing() const -> bool
{
    return GetToolId() != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::SourcePart() const -> QString
{
    return m_sourcePart;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::GetOriginPointId() const -> quint32
{
    return m_originId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::SetOriginPointId(quint32 value)
{
    m_originId = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::GetTargetPointId() const -> quint32
{
    return m_targetId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::SetTargetPointId(quint32 value)
{
    m_targetId = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::GetHostPartName() const -> QString
{
    return ui->comboBoxPiece->currentText();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::SetHostPartName(const QString &name)
{
    // Every part is offered here: the host part of an existing transfer is a fact to display, not a choice to
    // make — its combo is disabled for that reason. The list is built again from scratch, because this setter runs
    // once per parse — and the same dialog is offered its tool's options again when the tool is rebuilt under it
    // (see DialogTool::SurvivesFullParse) — so an item already there is not added a second time.
    ui->comboBoxPiece->clear();

    const QDomElement root = m_doc->documentElement();
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        const QString part = VDomDocument::GetParametrString(draw, AttrName);
        ui->comboBoxPiece->addItem(part, m_doc->PatternBlockMapper()->FindId(part));
    }

    const qint32 index = ui->comboBoxPiece->findText(name);
    if (index != -1)
    {
        ui->comboBoxPiece->setCurrentIndex(index); // fills the layers of that part
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::HostLayer() const -> QPair<quint32, QString>
{
    const QString text = ui->comboBoxLayer->currentText().trimmed();

    for (int i = 0; i < ui->comboBoxLayer->count(); ++i)
    {
        if (ui->comboBoxLayer->itemText(i) == text)
        {
            return {ui->comboBoxLayer->itemData(i).toUInt(), text}; // an existing layer of the host part
        }
    }

    // Not one of the offered names: the user is naming the layer the copies will be written into.
    return {NULL_ID, text};
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::GetHostLayerId() const -> quint32
{
    return HostLayer().first;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::GetHostLayerName() const -> QString
{
    return HostLayer().second;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODialogTransfert::IsValid() const -> bool
{
    if (ui->comboBoxPiece->currentIndex() == -1)
    {
        return false; // the portion has nowhere to land until a part is chosen
    }

    // An existing layer, or the name of the one to create: either way the copies have a layer to go to.
    return not HostLayer().second.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::ShowDialog(bool click)
{
    if (m_step == Step::Selecting && not click)
    {
        if (SourceObjects().isEmpty() && GetLineSources().isEmpty())
        {
            Refuse(tr("Select the objects to transfer first"));
            return;
        }

        m_step = Step::Origin;

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        scene->clearSelection();

        VAbstractValApplication::VApp()->getSceneView()->AllowRubberBand(false);

        // The copies are named after their sources: naming them here, while the source part is the active one,
        // is what lets the host part tell them apart.
        FillDefSourceNames(SourceObjects(), &data, "t"_L1);
        RefreshPreview();

        Say(tr("Click the merge point of the part to transfer"));
        emit ToolTip(tr("Click the merge point of the part to transfer"));
        return;
    }

    // Everything else is answered by a click on the canvas, which arrives in ChosenObject: a selection click,
    // the point X, or the point A.
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::ChosenObject(quint32 id, const SceneObject &type)
{
    switch (m_step)
    {
        case Step::Selecting:
            if (type == SceneObject::Line)
            {
                // A line is not a geometric object: it travels beside the portion, and its two endpoints decide
                // whether it can be transferred at all.
                ToggleLineSource(id);
            }
            else
            {
                ToggleSourceObject(id);
            }
            RefreshPreview(); // the picked objects light up as they are picked
            break;

        case Step::Origin:
            if (type != SceneObject::Point)
            {
                Refuse(tr("Click a point of the part: it is the point the portion is anchored on"));
                return;
            }

            m_originId = id;
            FillHostParts();

            if (ui->comboBoxPiece->count() == 0)
            {
                // No part at all: there is nowhere to put the portion. Say it now rather than open a dialog with
                // an empty list the user cannot answer.
                Refuse(tr("This pattern has no part that could receive the portion."));
                return;
            }

            m_step = Step::Host;
            Say(tr("Choose the part that receives the portion, and its layer"));
            // An ordinary window, not a modal one: hiding a modal dialog leaves the application inside a macOS
            // modal session, where nothing answers any more — not even the quit. Scene clicks are ignored while
            // this step is up, so there is nothing for modality to protect here.
            show();
            break;

        case Step::Host:
            // The dialog is up: the canvas does not answer for it.
            break;

        case Step::Target:
            if (type != SceneObject::Point)
            {
                Refuse(tr("Click the point of the host part that receives the merge point"));
                return;
            }

            m_targetId = id;

            // The gesture is over: the tool is created by the main window, in the part that is active now —
            // the host one, which is what puts the operation and its copies in the right part.
            DialogTool::DialogAccepted();
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::DialogAccepted()
{
    if (IsEditing())
    {
        DialogTool::DialogAccepted();
        return;
    }

    if (m_step != Step::Host)
    {
        // A second OK (or an OK after the switch) has nothing left to decide.
        emit ToolTip(tr("Click the point of the host part that receives the merge point"));
        return;
    }

    SaveData();

    const QString host = GetHostPartName();
    if (host.isEmpty())
    {
        Refuse(tr("Choose the part that receives the portion"));
        return;
    }

    // The switch itself: the main window makes the host part active — which moves the living container to its
    // snapshot, and shows it, since the canvas was still framing the source part.
    emit HostPartSelected(host);

    if (m_doc->PatternBlockMapper()->GetActive() != host)
    {
        Refuse(tr("The part %1 could not be activated").arg(host));
        return;
    }

    // The dialog is not destroyed: it holds the two merge points, and it is what will hand them to the tool.
    // Emitting DialogClosed here would apply the tool before A is known and unlink the dialog for good.
    hide();
    m_step = Step::Target;

    if (not vis.isNull())
    {
        // The portion belongs to the part that has just been left: its highlight is cleared so nothing stale is
        // drawn over the host part.
        auto *visual = qobject_cast<OVisToolTransfert *>(vis);
        SCASSERT(visual != nullptr)
        visual->SetObjects({});
        visual->SetPreviewLines({});
        visual->RefreshGeometry();
    }

    Say(tr("Click the point of the host part that receives the merge point"));
    emit ToolTip(tr("Click the point of the host part that receives the merge point"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::SaveData()
{
    // The list is the source of truth for the transferred objects: the canvas wrote into it, and this dialog
    // only ever removes from it.
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::HostPartChanged(int index)
{
    Q_UNUSED(index)
    FillHostLayers(ui->comboBoxPiece->currentText());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::FillHostParts()
{
    ui->comboBoxPiece->clear();

    // Every part of the pattern, in file order. It used to be the parts built AFTER the source only, because
    // the operation was read while its own block was parsed and the piece the portion comes from could not have
    // been read yet. A transfer whose host comes first is now kept aside and built once the whole file has been
    // read, so the order decides nothing any more — and file order is the one the parse follows.
    const QDomElement root = m_doc->documentElement();
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        const QString part = VDomDocument::GetParametrString(draw, AttrName);
        const int id = m_doc->PatternBlockMapper()->FindId(part);
        if (id != -1)
        {
            ui->comboBoxPiece->addItem(part, id);
        }
    }

    // Force a choice: the first part of the list is not a default, it is a decision.
    ui->comboBoxPiece->setCurrentIndex(-1);
    FillHostLayers(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::FillHostLayers(const QString &partName)
{
    ui->comboBoxLayer->clear();

    if (partName.isEmpty())
    {
        ui->comboBoxLayer->setEditText(QString());
        return;
    }

    const QMap<quint32, VGroupData> layers = m_doc->GetGroups(partName);
    quint32 active = NULL_ID;
    for (auto layer = layers.constBegin(); layer != layers.constEnd(); ++layer)
    {
        ui->comboBoxLayer->addItem(layer.value().name, layer.key());
        if (layer.value().active)
        {
            active = layer.key();
        }
    }

    // The layer the host part draws into is the one the copies would follow anyway: offering it first is what
    // the user expects, and it costs one line.
    const qint32 index = active != NULL_ID ? ui->comboBoxLayer->findData(active) : -1;
    if (index != -1)
    {
        ui->comboBoxLayer->setCurrentIndex(index);
    }
    else if (ui->comboBoxLayer->count() > 0)
    {
        ui->comboBoxLayer->setCurrentIndex(0);
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void ODialogTransfert::Refuse(const QString &message)
{
    emit ToolTip(message); // the status line keeps saying it once the box is gone

    // Over the main window, not over this dialog: the merge points are picked while the dialog is not on
    // screen. One button, and the step is still the one to answer — there is nothing to choose.
    QMessageBox::information(VAbstractValApplication::VApp()->getMainWindow(), tr("Transfer"), message);
}
