/************************************************************************
 **
 **  @file   opiecedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  New piece creation dialog (two steps).
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

#include "opiecedialog.h"

#include "opiecedialog_patternstep.h"
#include "opiecedialog_piecestep.h"
#include "../vtools/visualization/path/vistoolpiece.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vlayout/vlayoutpoint.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPieceDialog::OPieceDialog(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent)
{
    SCASSERT(doc != nullptr)

    vis = new VisToolPiece(&this->data);

    connect(&m_automation, &OPieceAutomation::ToolTipMessage, this,
            [this](const QString &msg) { emit ToolTip(msg); });
    connect(&m_automation, &OPieceAutomation::ListChanged, this, [this]() { BuildPreviewPiece(); });

    m_stack = new QStackedWidget(this);
    m_patternStep = new OPiecePatternStep(m_doc);
    m_pieceStep = new OPiecePieceStep();
    m_stack->addWidget(m_patternStep);
    m_stack->addWidget(m_pieceStep);
    m_stack->setCurrentWidget(m_pieceStep); // mono-step by default; ShowDialog picks the page

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    // Cancel aborts the whole tool: the collected contour is discarded, no piece is
    // created and the tool returns to idle (DialogRejected → DialogClosed(Rejected)
    // → CancelTool in the MainWindow). The pattern fields stay uncommitted — they are
    // only written on acceptance.
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_nextButton = new QPushButton(tr("Next"));
    m_okButton = new QPushButton(tr("OK"));
    bOk = m_okButton;
    m_cancelButton->setAutoDefault(false);
    m_nextButton->setAutoDefault(false);
    buttonRow->addWidget(m_cancelButton);
    buttonRow->addWidget(m_nextButton);
    buttonRow->addWidget(m_okButton);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_stack);
    mainLayout->addLayout(buttonRow);

    connect(m_cancelButton, &QPushButton::clicked, this, &DialogTool::DialogRejected);
    connect(m_nextButton, &QPushButton::clicked, this, &OPieceDialog::OnPatternNext);
    connect(m_okButton, &QPushButton::clicked, this, &OPieceDialog::DialogAccepted);

    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare)
    {
        return;
    }

    m_automation.HandleObjectChosen(id, type);
    BuildPreviewPiece();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::ShowDialog(bool click)
{
    if (click || m_automation.NodeCount() == 0)
    {
        return;
    }

    emit ToolTip(QString());
    prepare = true;

    // The contour is complete: resolve the sense of curves whose entry was unknown,
    // then complete a closing edge that follows an already used curve.
    m_automation.ReconcilePendingReverse();
    m_automation.CompleteClosingCurve(false);
    if (not m_automation.PendingReverseCurves().isEmpty())
    {
        m_automation.ShowSenseMessage();
    }
    m_automation.ClearPendingReverse();

    auto *visPath = qobject_cast<VisToolPiece *>(vis);
    SCASSERT(visPath != nullptr)
    visPath->SetMode(Mode::Show);
    visPath->RefreshGeometry();

    // First piece → two steps; later pieces → piece step only (pattern data is fixed).
    m_stack->setCurrentWidget(data.DataPieces()->isEmpty() ? static_cast<QWidget *>(m_patternStep)
                                                           : static_cast<QWidget *>(m_pieceStep));
    // Piece number total: existing effective pieces (standalone + groups) + the one being created.
    m_pieceStep->SetPieceTotal(data.GetEffectivePieceCount() + 1);
    UpdateButtons();

    // The contour is collected and frozen: state the refusal reason inside the dialog itself, next to the
    // greyed OK button that it explains.
    m_pieceStep->SetContourProblem(ContourProblem());

    // Fix issue #526. Dialog Detail is not on top after selection second object on Mac.
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::SetPatternDoc(VAbstractPattern *doc)
{
    SCASSERT(doc != nullptr)
    m_doc = doc;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::CheckDependencyTreeComplete()
{
    // does nothing
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDialog::IsValid() const -> bool
{
    // The OK button is enabled only for a contour that can really be used as a piece.
    return ContourProblem().isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ContourProblem tells why the collected contour cannot be used as a piece, and returns an empty string
 * when it can.
 *
 * A piece is built on its contour: a contour whose curve cannot be computed, or whose points are too few, or
 * which runs counter-clockwise, gives a piece whose seam allowance and inside are swapped. Such a piece must
 * never be created — and when it is refused, the reason has to be said, otherwise the disabled OK button leaves
 * the user with no way out.
 */
auto OPieceDialog::ContourProblem() const -> QString
{
    const VPiece piece = GetPiece();
    const VPiecePath path = piece.GetPath();

    // Every curve of the contour must be able to give the segment between the points around it.
    QString curve;
    if (path.FindUnresolvableCurve(&data, curve))
    {
        return tr("Cannot calculate curve %1.").arg(curve);
    }

    QVector<QPointF> points;
    CastTo(piece.MainPathPoints(&data), points);

    if (points.count() < 3)
    {
        return tr("You need more points!");
    }

    if (VPiece::SumTrapezoids(points) >= 0)
    {
        return tr("You have to choose points in a clockwise direction!");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDialog::GetPiece() const -> VPiece
{
    VPiece piece = data.DataPieces()->value(toolId); // options the dialog does not control

    piece.GetPath().SetNodes(m_automation.Nodes());

    // Drawing default from the pattern seed, which itself falls back to the application setting.
    VCommonSettings *settings = VAbstractValApplication::VApp()->Settings();
    piece.SetSewLineOnDrawing(m_doc->GetDefaultSewLineOnDrawing());

    // Pattern seeds (Activate SA + Hide main path + sew line on drawing) apply to every piece created afterwards.
    piece.SetSeamAllowance(m_doc->GetDefaultActivateSA());
    piece.SetHideMainPath(m_doc->GetDefaultHideMainPath());
    piece.SetSecondPassmarkOnSeamLine(m_doc->GetDefaultSecondPassmarkOnSeamLine());
    if (piece.IsSeamAllowance())
    {
        // Convert the default seam allowance from the app unit to the pattern unit,
        // mirroring DialogSeamAllowance::InitSeamAllowanceTab.
        const double defaultSA = UnitConvertor(settings->GetDefaultSeamAllowance(), StrToUnits(settings->GetUnit()),
                                               VAbstractValApplication::VApp()->patternUnits());
        piece.SetFormulaSAWidth(QString::number(defaultSA), defaultSA);
    }

    // Piece name is required (checked on acceptance); the preview may still hold an
    // empty name while the contour is being collected.
    piece.SetName(m_pieceStep->GetPieceName());
    piece.SetPieceNumber(m_pieceStep->GetPieceNumber());
    piece.SetCutFabricQty(m_pieceStep->GetCutFabricQty());
    piece.SetCutFabricOnFold(m_pieceStep->IsCutFabricOnFold());
    piece.SetCutInterfacingQty(m_pieceStep->GetCutInterfacingQty());
    piece.SetCutInterfacingOnFold(m_pieceStep->IsCutInterfacingOnFold());
    piece.SetCutLiningQty(m_pieceStep->GetCutLiningQty());
    piece.SetCutLiningOnFold(m_pieceStep->IsCutLiningOnFold());
    piece.SetFreeText(m_pieceStep->GetFreeText());
    piece.SetGradedAlignH(m_pieceStep->GetGradedAlignH());
    piece.SetGradedAlignV(m_pieceStep->GetGradedAlignV());

    // Layout flags, as chosen in the step-2 dialog (symmetrical copy is checked by default).
    piece.SetForceFlipping(m_pieceStep->IsForceFlipping());
    piece.SetSymmetricalCopy(m_pieceStep->IsSymmetricalCopy());

    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::BuildPreviewPiece()
{
    auto *visPath = qobject_cast<VisToolPiece *>(vis);
    SCASSERT(visPath != nullptr)
    const VPiece piece = GetPiece();
    visPath->SetPiece(piece);

    if (piece.GetPath().CountNodes() == 1)
    {
        emit ToolTip(tr("Select main path objects clockwise, <b>%1</b> - finish creation")
                         .arg(VModifierKey::EnterKey()));

        if (not VAbstractValApplication::VApp()->getCurrentScene()->items().contains(visPath))
        {
            visPath->VisualMode(NULL_ID);
        }
        else
        {
            visPath->RefreshGeometry();
        }
    }
    else
    {
        visPath->RefreshGeometry();

        // Say what is still missing, otherwise the disabled OK button is a dead end. While the direction of a
        // curve is not settled yet the automation owns the message, so leave it alone in that case.
        if (m_automation.PendingReverseCurves().isEmpty())
        {
            emit ToolTip(ContourProblem()); // empty when the contour is usable: clears an outdated message
        }
    }

    // The same reason, written inside the dialog: the tooltip is not shown any more once the dialog is open.
    m_pieceStep->SetContourProblem(ContourProblem());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::UpdateButtons()
{
    // Cancel stays available on both pages: aborting the tool is always possible.
    const bool patternPage = m_stack->currentWidget() == m_patternStep;
    m_nextButton->setVisible(patternPage);
    m_okButton->setVisible(not patternPage);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::OnPatternNext()
{
    // Pattern fields are committed on acceptance (DialogAccepted), not here:
    // Next alone must not leave the document modified if the user then cancels.
    m_patternCommitted = true;
    m_stack->setCurrentWidget(m_pieceStep);
    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDialog::DialogAccepted()
{
    // Never create a piece on a contour that cannot be used. The OK button is already disabled in that case;
    // this covers the keyboard and any programmatic acceptance, and it says why instead of doing nothing.
    if (const QString problem = ContourProblem(); not problem.isEmpty())
    {
        emit ToolTip(problem);
        return;
    }

    // Piece name is required: highlight the missing field instead of silently
    // auto-naming the piece.
    if (m_pieceStep->GetPieceName().isEmpty())
    {
        m_pieceStep->SetPieceNameInvalid(true);
        return;
    }

    if (m_patternCommitted)
    {
        m_doc->SetPatternName(m_patternStep->GetPatternName());
        m_doc->SetCustomer(m_patternStep->GetClient());
        m_doc->SetCollection(m_patternStep->GetCollection());
        m_doc->SetFabricInfo(m_patternStep->GetFabricInfo());
        m_doc->SetDefaultActivateSA(m_patternStep->IsActivateSA());
        m_doc->SetDefaultHideMainPath(m_patternStep->IsHideMainPath());
        m_doc->SetDefaultSewLineOnDrawing(m_patternStep->IsSewLineOnDrawing());
        m_doc->SetDefaultSecondPassmarkOnSeamLine(m_patternStep->IsSecondPassmarkOnSeamLine());
        // Information block: the chosen template (empty when the user did not opt in).
        m_doc->SetCartoucheTemplate(m_patternStep->GetCartouche());
    }
    DialogTool::DialogAccepted();
}
