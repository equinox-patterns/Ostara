/************************************************************************
 **
 **  @file   odetailspiecewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Piece properties widget for the Details mode options dock
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

#include "odetailspiecewidget.h"

#include "oannotationsconfigwidget.h"
#include "oblockconfigwidget.h"
#include "odetailsfoldlinewidget.h"
#include "odetailsgrainlinewidget.h"
#include "odetailsinternalpathwidget.h"
#include "odetailsseamallowancewidget.h"
#include "opieceinfowidget.h"
#include "othumbnailconfigwidget.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/ochildanchor.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vgeometry/vpointf.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vtools/dialogs/tools/piece/dialogmainpath.h"
#include "../vtools/dialogs/tools/piece/dialogpiecepath.h"
#include "../vtools/dialogs/tools/piece/dialogpiecepathoptions.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/deltool.h"
#include "../vtools/undocommands/savepiecegroupoptions.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "../vtools/undocommands/savepiecepathoptions.h"
#include "../vexception/vexception.h"
#include "../vtheme/vtheme.h"
#include "otranslation.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSize>
#include <QTabWidget>
#include <QTimer>
#include <QUndoStack>
#include <QVariant>
#include <QVBoxLayout>

namespace
{
// Graded alignment icons: regular (outline) for the normal state, fill for the checked state.
// QCommonStyle renders a checked QPushButton icon in QIcon::On, unchecked in QIcon::Off.
auto MakeAlignIcon(const QString &baseName, bool dark) -> QIcon
{
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, baseName));                    // Off = regular
    icon.addFile(QStringLiteral(":/icons/%1/%2-fill_%1.svg").arg(theme, baseName), QSize(),
                 QIcon::Normal, QIcon::On);                                                       // On = fill
    // Disabled = the greyed-out regular set. The row is always unchecked before it is disabled
    // (forced alignment point, group member following its group), so Off is the only state needed.
    icon.addFile(QStringLiteral(":/icons/%1/%2_disabled.svg").arg(theme, baseName), QSize(),
                 QIcon::Disabled, QIcon::Off);
    return icon;
}

// Hover icon: the button background turns accent on hover (QSS QPushButton:hover), and on an
// accent background the icon must always be the Dark fill set (Icons.md "fond accent" rule),
// whatever the active theme. Both states map to the same file.
auto MakeAlignHoverIcon(const QString &baseName) -> QIcon
{
    const QString file = QStringLiteral(":/icons/dark/%1-fill_dark.svg").arg(baseName);
    QIcon icon;
    icon.addFile(file);                                    // Off = fill dark
    icon.addFile(file, QSize(), QIcon::Normal, QIcon::On); // On = fill dark
    return icon;
}

// alignValue 1/2/3 → Left/Center/Right horizontally, Top/Center/Bottom vertically.
auto AlignIconBaseName(bool vertical, int value) -> QString
{
    switch (value)
    {
        case 1:
            return vertical ? QStringLiteral("align-top-simple") : QStringLiteral("align-left-simple");
        case 3:
            return vertical ? QStringLiteral("align-bottom-simple") : QStringLiteral("align-right-simple");
        default:
            return vertical ? QStringLiteral("align-center-vertical-simple")
                            : QStringLiteral("align-center-horizontal-simple");
    }
}

struct AlignItem
{
    int value;
    const char *text;
};

// Shared by the constructor and RetranslateUI() — QT_TRANSLATE_NOOP lets lupdate extract them.
const AlignItem kAlignHItems[] = {
    {1, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Left")},
    {2, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Right")},
};

const AlignItem kAlignVItems[] = {
    {1, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Top")},
    {2, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Bottom")},
};

// Outer-tab indices of the piece options (order of addTab). Each tab hosts its own
// node-picking workflow: passmarks (General), fold line (Grainline), custom seam
// allowance + the "Internal outline" list (Seam Allowance).
constexpr int kGeneralTabIndex = 0;
constexpr int kGrainlineTabIndex = 1;
constexpr int kSeamAllowanceTabIndex = 2;

// First sub-tab of the Information tab (Piece / Blocks / Annotations). A piece opens there.
constexpr int kPieceInfoSubTabIndex = 0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsPieceWidget::ODetailsPieceWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Debounced tab re-population (see the indexChanged connect in ConnectControls): a drag
    // pushes a MovePiece per mouse move and each push fires indexChanged, so rebuilding the tabs
    // immediately would flicker their lists. The single-shot collapses the burst into one refresh
    // once the interaction settles, while keeping undo/redo updates prompt.
    m_repopulateTimer = new QTimer(this);
    m_repopulateTimer->setSingleShot(true);
    m_repopulateTimer->setInterval(100);
    connect(m_repopulateTimer, &QTimer::timeout, this, [this]()
    {
        if (!m_repopulateTool)
        {
            // The captured tool died. Clear the stale page only when nothing else is selected;
            // a replacement tool already repopulated it.
            if (!m_currentTool)
            {
                Clear();
            }
            return;
        }
        if (m_repopulateTool != m_currentTool)
        {
            // The selection moved to another tool: the current page is up to date.
            return;
        }
        const QPointer<VToolSeamAllowance> currentTool = m_repopulateTool;
        if (not currentTool->getData()->DataPieces()->contains(currentTool->getId()))
        {
            // The piece was deleted: nothing to refresh.
            return;
        }
        const VPiece detail = currentTool->getData()->GetPiece(currentTool->getId());
        m_pendingDet = detail;
        m_pieceInfoWidget->Show(detail, currentTool->getData());
        m_grainlineWidget->Show(detail.GetGrainlineGeometry(),
                                [this, detail, currentTool]() -> std::optional<QPointF>
        {
            // detail captured by value: the provider outlives this lambda, so a reference would
            // dangle and crash on the next grainline activation.
            return GrainlineCenter(detail, currentTool->getData());
        });
        m_foldlineWidget->Show(detail, currentTool->getData(), currentTool->getId());
        m_saWidget->RefreshList(detail);
        if (m_internalPathWidget->RefreshList(detail, currentTool->getData()))
        {
            // The active path disappeared (undo/redo of a delete) — clear the canvas.
            emit ClearHighlight();
        }
        m_blockConfigWidget->Show(detail, currentTool->getData());
        m_annotationsConfigWidget->Show(detail, currentTool->getData());
        m_thumbnailConfigWidget->Show(detail.GetThumbnailData(), currentTool->getData(), currentTool->getId());
    });

    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName(QStringLiteral("detailsOptionsTabWidget"));

    // ── General tab ──────────────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);

        // Graded alignment
        m_alignGroupBox = new QGroupBox(tr("Graded alignment"));
        auto *alignLayout = new QVBoxLayout(m_alignGroupBox);

        // Horizontal
        m_alignHLabel = new QLabel(tr("Horizontal"));
        auto *hLayout = new QHBoxLayout();
        m_alignHGroup = new QButtonGroup(this);
        m_alignHGroup->setExclusive(true);

        for (int i = 0; i < 3; ++i)
        {
            auto *btn = new QPushButton(tr(kAlignHItems[i].text));
            btn->setCheckable(true);
            btn->setProperty("alignValue", kAlignHItems[i].value);
            btn->installEventFilter(this); // hover icon swap
            connect(btn, &QAbstractButton::toggled, this, [btn](bool checked) {
                // A selected button has no hover: restore its theme rendering under the cursor.
                if (not btn->underMouse())
                {
                    return;
                }
                const QVariant prop = checked ? btn->property("normalIcon") : btn->property("hoverIcon");
                if (prop.isValid())
                {
                    btn->setIcon(prop.value<QIcon>());
                }
            });
            m_alignHGroup->addButton(btn, kAlignHItems[i].value);
            hLayout->addWidget(btn);
            m_alignHBtns[i] = btn;
        }
        alignLayout->addWidget(m_alignHLabel);
        alignLayout->addLayout(hLayout);

        // Vertical
        m_alignVLabel = new QLabel(tr("Vertical"));
        auto *vLayout = new QHBoxLayout();
        m_alignVGroup = new QButtonGroup(this);
        m_alignVGroup->setExclusive(true);

        for (int i = 0; i < 3; ++i)
        {
            auto *btn = new QPushButton(tr(kAlignVItems[i].text));
            btn->setCheckable(true);
            btn->setProperty("alignValue", kAlignVItems[i].value);
            btn->installEventFilter(this); // hover icon swap
            connect(btn, &QAbstractButton::toggled, this, [btn](bool checked) {
                // A selected button has no hover: restore its theme rendering under the cursor.
                if (not btn->underMouse())
                {
                    return;
                }
                const QVariant prop = checked ? btn->property("normalIcon") : btn->property("hoverIcon");
                if (prop.isValid())
                {
                    btn->setIcon(prop.value<QIcon>());
                }
            });
            m_alignVGroup->addButton(btn, kAlignVItems[i].value);
            vLayout->addWidget(btn);
            m_alignVBtns[i] = btn;
        }
        alignLayout->addWidget(m_alignVLabel);
        alignLayout->addLayout(vLayout);

        // Forced alignment point: overrides the horizontal row above with a main path node whose X
        // is shared by every graded size. The vertical row keeps working.
        auto *alignPointSep = new QFrame(m_alignGroupBox);
        alignPointSep->setFrameShape(QFrame::HLine);
        alignPointSep->setFrameShadow(QFrame::Sunken);
        alignLayout->addWidget(alignPointSep);

        m_alignPointBtn = new QPushButton(tr("Align on a point"));
        alignLayout->addWidget(m_alignPointBtn);

        m_removeAlignPointBtn = new QPushButton(tr("Remove point"));
        m_removeAlignPointBtn->setVisible(false); // only once a point exists
        alignLayout->addWidget(m_removeAlignPointBtn);

        m_showAlignPointCheck = new QCheckBox(tr("Show alignment point"));
        alignLayout->addWidget(m_showAlignPointCheck);

        layout->addWidget(m_alignGroupBox);

        // Layout flags — flipping & grainline constraints. Same flags and texts as the piece
        // creation dialog step 2; this is where the choice made at creation can be changed.
        auto *flagsSeparator = new QFrame();
        flagsSeparator->setFrameShape(QFrame::HLine);
        flagsSeparator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(flagsSeparator);

        m_forceFlippingCheck = new QCheckBox(tr("Force flipping"));
        m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
        layout->addWidget(m_forceFlippingCheck);

        m_symmetricalCopyCheck = new QCheckBox(tr("Symmetrical copy"));
        m_symmetricalCopyCheck->setToolTip(tr("When the number of copies is greater than 1 and 'Force flipping' is selected, each even-numbered copy of the piece will be flipped vertically."));
        layout->addWidget(m_symmetricalCopyCheck);

        auto *flagsEndSeparator = new QFrame();
        flagsEndSeparator->setFrameShape(QFrame::HLine);
        flagsEndSeparator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(flagsEndSeparator);

        // Passmark multi-selection
        auto *passmarkRow = new QHBoxLayout();
        m_addPassmarksBtn = new QPushButton(tr("+ Add passmarks"));
        passmarkRow->addWidget(m_addPassmarksBtn);

        // One Remove mode for every mark of the piece: passmarks and internal marks alike.
        m_removePassmarksBtn = new QPushButton(tr("- Remove marks"));
        passmarkRow->addWidget(m_removePassmarksBtn);
        layout->addLayout(passmarkRow);

        // Manage main path
        m_manageMainPathBtn = new QPushButton(tr("Manage main path"));
        layout->addWidget(m_manageMainPathBtn);

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("General"));
    }

    // ── Grainline tab ────────────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);

        m_grainlineWidget = new ODetailsGrainlineWidget(tab);
        layout->addWidget(m_grainlineWidget);

        // Separator
        auto *separator = new QFrame();
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(separator);

        m_foldlineWidget = new ODetailsFoldLineWidget(tab);
        layout->addWidget(m_foldlineWidget);

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("Grainline"));
    }

    // ── Seam Allowance tab ───────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);

        m_saWidget = new ODetailsSeamAllowanceWidget(tab);
        layout->addWidget(m_saWidget);

        // Internal outline list — lives in the Seam Allowance tab, right below the "Manage saved
        // custom seam allowances" button (the last control of the SA widget).
        auto *separator = new QFrame(tab);
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(separator);

        // ⚠️ **Le poseur, et non `tr()` seul.** Ce libellé était le seul texte de la classe sans
        // référence conservée : `RetranslateUI()` réappliquait les douze autres et ne pouvait pas
        // l'atteindre. Le poseur s'en passe — il pose la traduction et garde la source sur le
        // widget, que le moteur rejoue à chaque bascule.
        auto *internalOutlineLabel = new QLabel(tab);
        OTranslation::SetText(this, internalOutlineLabel, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Internal outline"));
        internalOutlineLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(internalOutlineLabel);

        m_internalPathWidget = new ODetailsInternalPathWidget(tab);
        layout->addWidget(m_internalPathWidget, 0, Qt::AlignTop); // hug the top, do not stretch

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("Seam Allowance"));
    }

    // ── Information tab — sub-tabs: Piece, Blocks, Annotations, Thumbnail. ──
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);
        layout->setContentsMargins(0, 0, 0, 0);

        m_infoSubTabs = new QTabWidget();
        m_infoSubTabs->setObjectName(QStringLiteral("informationTabWidget"));
        layout->addWidget(m_infoSubTabs);

        // Piece sub-tab (index 0) — moved out of the General tab: these fields
        // are filled through the step-2 dialog, no need to keep them constantly
        // under the user's eyes. Same structure as the Blocks tab: the tab
        // content IS the container (QFrame#detailsOptionsTabPage, §14 — the
        // DockBody background of the group boxes), with the fields directly
        // inside and the container margins.
        auto *pieceTab = new QFrame(m_infoSubTabs);
        pieceTab->setFrameStyle(QFrame::NoFrame);
        pieceTab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *pieceLayout = new QVBoxLayout(pieceTab);
        pieceLayout->setContentsMargins(8, 8, 8, 0);
        pieceLayout->setSpacing(8);

        auto *form = new QFormLayout();
        m_nameEdit = new QLineEdit(pieceTab);
        form->addRow(tr("Name"), m_nameEdit);
        m_nameLabel = qobject_cast<QLabel *>(form->labelForField(m_nameEdit));
        pieceLayout->addLayout(form);

        m_pieceInfoWidget = new OPieceInfoWidget(pieceTab);
        pieceLayout->addWidget(m_pieceInfoWidget);
        pieceLayout->addStretch(1);

        m_infoSubTabs->addTab(pieceTab, tr("Piece"));

        m_blockConfigWidget = new OBlockConfigWidget(m_infoSubTabs);
        m_infoSubTabs->addTab(m_blockConfigWidget, tr("Blocks"));

        m_annotationsConfigWidget = new OAnnotationsConfigWidget(m_infoSubTabs);
        m_infoSubTabs->addTab(m_annotationsConfigWidget, tr("Annotations"));

        m_thumbnailConfigWidget = new OThumbnailConfigWidget(m_infoSubTabs);
        m_infoSubTabs->addTab(m_thumbnailConfigWidget, tr("Thumbnail"));

        m_tabWidget->addTab(tab, tr("Information"));
    }

    mainLayout->addWidget(m_tabWidget);

    ConnectControls();

    RefreshAlignIcons();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::RetranslateUI()
{
    // General tab
    if (m_nameLabel)
    {
        m_nameLabel->setText(tr("Name"));
    }
    m_pieceInfoWidget->RetranslateUI();
    if (m_alignGroupBox)
    {
        m_alignGroupBox->setTitle(tr("Graded alignment"));
    }
    if (m_alignHLabel)
    {
        m_alignHLabel->setText(tr("Horizontal"));
    }
    if (m_alignVLabel)
    {
        m_alignVLabel->setText(tr("Vertical"));
    }
    for (int i = 0; i < 3; ++i)
    {
        m_alignHBtns[i]->setText(tr(kAlignHItems[i].text));
        m_alignVBtns[i]->setText(tr(kAlignVItems[i].text));
    }
    if (m_alignPointBtn)
    {
        m_alignPointBtn->setText(m_alignPointSelectState == AlignPointSelectState::Selecting
                                     ? tr("Cancel")
                                     : tr("Align on a point"));
        m_removeAlignPointBtn->setText(tr("Remove point"));
        m_showAlignPointCheck->setText(tr("Show alignment point"));
    }
    m_manageMainPathBtn->setText(tr("Manage main path"));
    m_addPassmarksBtn->setText(m_passmarkSelectState == PassmarkSelectState::Adding ? tr("Cancel")
                                                                                     : tr("+ Add passmarks"));
    m_removePassmarksBtn->setText(m_passmarkSelectState == PassmarkSelectState::Removing
                                      ? tr("Cancel")
                                      : tr("- Remove marks"));
    if (m_forceFlippingCheck)
    {
        m_forceFlippingCheck->setText(tr("Force flipping"));
        // ⚠️ Les deux cases portent aussi une infobulle, posée à la construction du panneau. Une
        // infobulle n'est pas un widget : la classe de base ne la retouche jamais, donc sans ces deux
        // lignes la page Piece gardait les explications dans la langue du lancement.
        m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
    }
    if (m_symmetricalCopyCheck)
    {
        m_symmetricalCopyCheck->setText(tr("Symmetrical copy"));
        m_symmetricalCopyCheck->setToolTip(
            tr("When the number of copies is greater than 1 and 'Force flipping' is selected, each "
               "even-numbered copy of the piece will be flipped vertically."));
    }
    m_tabWidget->setTabText(0, tr("General"));
    m_tabWidget->setTabText(1, tr("Grainline"));
    m_tabWidget->setTabText(2, tr("Seam Allowance"));
    m_tabWidget->setTabText(3, tr("Information"));
    m_infoSubTabs->setTabText(0, tr("Piece"));
    m_infoSubTabs->setTabText(1, tr("Blocks"));
    m_infoSubTabs->setTabText(2, tr("Annotations"));
    m_infoSubTabs->setTabText(3, tr("Thumbnail"));

    // Sub-widgets (Grainline tab, Fold line, Seam Allowance, Internal path)
    m_grainlineWidget->RetranslateUI();
    m_foldlineWidget->RetranslateUI();
    m_saWidget->RetranslateUI();
    m_internalPathWidget->RetranslateUI();
    m_blockConfigWidget->RetranslateUI();
    m_annotationsConfigWidget->RetranslateUI();
    m_thumbnailConfigWidget->RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ConnectControls()
{
    // Sub-widgets commit through the same snapshot + undo pipeline as the General tab.
    m_grainlineWidget->SetModifyPending([this](std::function<void(VGrainlineData &)> func)
    {
        ModifyPiece([func](VPiece &det) { func(det.GetGrainlineGeometry()); });
    });
    m_foldlineWidget->SetModifyPending([this](std::function<void(VPiece &)> func) { ModifyPiece(func); });
    m_pieceInfoWidget->SetModifyPending([this](std::function<void(IPieceInfo &)> func) { ModifyPiece(func); });
    connect(m_foldlineWidget, &ODetailsFoldLineWidget::ShowStatusTip, this, &ODetailsPieceWidget::ShowStatusTip);
    connect(m_foldlineWidget, &ODetailsFoldLineWidget::HighlightNode, this,
            [this](quint32 nodeId, bool highlight)
    {
        emit HighlightNode(nodeId, highlight);
    });

    m_saWidget->SetModifyPending([this](std::function<void(VPiece &)> func) { ModifyPiece(func); });
    m_blockConfigWidget->SetModifyPending([this](std::function<void(IPieceInfo &)> func) { ModifyPiece(func); });
    m_annotationsConfigWidget->SetModifyPending([this](std::function<void(IPieceInfo &)> func) { ModifyPiece(func); });
    m_thumbnailConfigWidget->SetModifyPending([this](std::function<void(OThumbnailData &)> func)
    {
        // The miniature editor speaks of the miniature alone; the piece page turns that into a piece
        // mutation, which is what the undo command and the XML writer expect. The type is spelled out:
        // a lambda taking IPieceInfo& fits both ModifyPiece overloads, and the call would be ambiguous.
        const std::function<void(IPieceInfo &)> mutate = [&func](IPieceInfo &det)
        {
            OThumbnailData data = det.GetThumbnailData();
            func(data);
            det.SetThumbnailData(data);
        };
        ModifyPiece(mutate);
    });
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::ShowStatusTip, this, &ODetailsPieceWidget::ShowStatusTip);
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::HighlightNode, this,
            [this](quint32 nodeId, bool highlight, QColor color)
    {
        emit HighlightNode(nodeId, highlight, color);
    });
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::ClearHighlight, this, &ODetailsPieceWidget::ClearHighlight);

    connect(m_internalPathWidget, &ODetailsInternalPathWidget::ShowStatusTip, this,
            &ODetailsPieceWidget::ShowStatusTip);
    connect(m_internalPathWidget, &ODetailsInternalPathWidget::EditRequested, this,
            &ODetailsPieceWidget::OnEditInternalPath);
    connect(m_internalPathWidget, &ODetailsInternalPathWidget::DeleteRequested, this,
            &ODetailsPieceWidget::DeleteInternalPath);
    connect(m_internalPathWidget, &ODetailsInternalPathWidget::InternalPathActivated, this,
            &ODetailsPieceWidget::OnInternalPathActivated);

    // Leaving a tab drops the node-picking workflow it hosts: its "Cancel" button (passmarks,
    // fold line, custom seam allowance) would otherwise stay armed while the user is on another
    // tab, and canvas node clicks would keep routing to the hidden workflow. Same rule for the
    // active internal-outline row, which lives in the Seam Allowance tab.
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index)
    {
        if (index != kGeneralTabIndex && IsSelectingPassmarkPoints())
        {
            CancelPassmarkSelection();
        }
        if (index != kGrainlineTabIndex && IsSelectingFoldLinePoints())
        {
            CancelFoldLineSelection();
        }
        if (index != kSeamAllowanceTabIndex)
        {
            ClearInternalPathActivation();
            CancelSASelection(); // internally guarded: no-op when no SA selection is running
        }
        // The alignment point belongs to the General tab: leaving it drops the picking workflow,
        // and the orange point only lives while its own tab is on screen.
        if (index != kGeneralTabIndex)
        {
            CancelAlignPointSelection();
        }
        RefreshAlignPointHighlight();
    });

    // Re-populate the tabs after undo/redo, debounced: a drag pushes a MovePiece per mouse move
    // (each fires indexChanged), so rebuilding immediately would flicker the lists; the timer
    // collapses the burst into one refresh once the interaction settles. The deferred run also
    // avoids re-entrancy while CommitPending() is mid-flight (indexChanged fires synchronously
    // during push). The tool is snapshotted into m_repopulateTool at schedule time; the timeout
    // lambda (in the constructor) re-checks it against m_currentTool.
    connect(VAbstractApplication::VApp()->getUndoStack(), &QUndoStack::indexChanged, this, [this](int)
    {
        m_repopulateTool = m_currentTool;
        m_repopulateTimer->start();
    });

    // ── General tab connections ──────────────────────────────────

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]()
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        m_pendingDet.SetName(m_nameEdit->text());
        CommitPending();
    });

    connect(m_alignHGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const int v = button->property("alignValue").toInt();
        ModifyPiece([v](VPiece &det) { det.SetGradedAlignH(v); });
    });

    connect(m_alignVGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const int v = button->property("alignValue").toInt();
        ModifyPiece([v](VPiece &det) { det.SetGradedAlignV(v); });
    });

    // Forced alignment point: pick it on the piece outline, or drop it.
    connect(m_alignPointBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnAlignPointClicked);
    connect(m_removeAlignPointBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnRemoveAlignPoint);
    connect(m_showAlignPointCheck, &QCheckBox::toggled, this, &ODetailsPieceWidget::OnShowAlignPointToggled);

    // Graded alignment icons follow the current theme (Light/Dark toggle).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &ODetailsPieceWidget::RefreshAlignIcons);

    // ── Layout flags (flipping & grainline) ──────────────────────

    connect(m_forceFlippingCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        ModifyPiece([checked](VPiece &det) { det.SetForceFlipping(checked); });
    });

    connect(m_symmetricalCopyCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        ModifyPiece([checked](VPiece &det) { det.SetSymmetricalCopy(checked); });
    });

    // ── Passmark multi-selection ──────────────────────────────────

    connect(m_addPassmarksBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnAddPassmarks);
    connect(m_removePassmarksBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnRemovePassmarks);

    // ── Manage main path ──────────────────────────────────────────

    connect(m_manageMainPathBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnManageMainPath);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ModifyPiece(std::function<void(VPiece &)> func)
{
    if (m_updating || !m_currentTool)
    {
        return;
    }
    func(m_pendingDet);
    CommitPending();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ModifyPiece(std::function<void(IPieceInfo &)> func)
{
    if (m_updating || !m_currentTool)
    {
        return;
    }
    // VPiece is-a IPieceInfo: the piece-like mutation applies to the pending detail.
    func(m_pendingDet);
    CommitPending();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::RefreshAlignIcons()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QSize iconSize(16, 16);
    for (int i = 0; i < 3; ++i)
    {
        const QIcon hNormal = MakeAlignIcon(AlignIconBaseName(false, i + 1), dark);
        m_alignHBtns[i]->setIcon(hNormal);
        m_alignHBtns[i]->setIconSize(iconSize);
        m_alignHBtns[i]->setProperty("normalIcon", QVariant::fromValue(hNormal));
        m_alignHBtns[i]->setProperty("hoverIcon",
                                     QVariant::fromValue(MakeAlignHoverIcon(AlignIconBaseName(false, i + 1))));

        const QIcon vNormal = MakeAlignIcon(AlignIconBaseName(true, i + 1), dark);
        m_alignVBtns[i]->setIcon(vNormal);
        m_alignVBtns[i]->setIconSize(iconSize);
        m_alignVBtns[i]->setProperty("normalIcon", QVariant::fromValue(vNormal));
        m_alignVBtns[i]->setProperty("hoverIcon",
                                     QVariant::fromValue(MakeAlignHoverIcon(AlignIconBaseName(true, i + 1))));
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ODetailsPieceWidget::eventFilter(QObject *obj, QEvent *event)
{
    // Hover icon swap for the graded alignment buttons: an unselected button gets the dark-fill
    // hover icon on enter (accent background), a selected button has no hover.
    if (event->type() == QEvent::Enter)
    {
        auto *btn = qobject_cast<QPushButton *>(obj);
        if (btn && btn->isEnabled() && not btn->isChecked() && btn->property("hoverIcon").isValid())
        {
            btn->setIcon(btn->property("hoverIcon").value<QIcon>());
        }
    }
    else if (event->type() == QEvent::Leave)
    {
        auto *btn = qobject_cast<QPushButton *>(obj);
        if (btn && btn->property("normalIcon").isValid())
        {
            btn->setIcon(btn->property("normalIcon").value<QIcon>());
        }
    }
    return QWidget::eventFilter(obj, event);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::GrainlineCenter(const VPiece &det, const VContainer *data) -> std::optional<QPointF>
{
    const QVector<VPointF> points = det.MainPathNodePoints(data);
    if (points.isEmpty())
    {
        return std::nullopt;
    }
    QRectF bbox;
    for (const auto &p : points)
    {
        const QPointF pt = p.toQPointF();
        bbox = bbox.isNull() ? QRectF(pt, QSizeF(0, 0)) : bbox.united(QRectF(pt, QSizeF(0, 0)));
    }
    // Local center of the piece bbox (the grainline is a child of the piece).
    return bbox.center() - QPointF(det.GetMx(), det.GetMy());
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::Show(VToolSeamAllowance *tool)
{
    if (!tool)
    {
        Clear();
        return;
    }

    const VPiece detail = tool->getData()->GetPiece(tool->getId());

    const quint32 previousPieceId = m_pieceId;
    m_currentTool = tool;
    m_pieceId = tool->getId();
    m_pendingDet = detail;

    // A piece opens on its default tabs, whatever tab was left open on the previous piece: this widget
    // instance is reused for every piece and QTabWidget keeps its index. Repopulating the same piece
    // — undo, commit, parse refresh — leaves the tab the user is working in untouched. The targeted
    // openings (grainline, block, annotation) set their tab after Show(), so they still win.
    if (previousPieceId != m_pieceId)
    {
        m_tabWidget->setCurrentIndex(kGeneralTabIndex);
        m_infoSubTabs->setCurrentIndex(kPieceInfoSubTabIndex);
        m_pieceInfoWidget->ResetTabs(); // the cut tabs (Fabric / Interfacing / Lining) too
        m_alignPointSelectState = AlignPointSelectState::Idle; // never carry a pick across pieces
        m_showAlignPointState = true;                          // the "show" checkbox opens checked
    }

    m_updating = true;

    m_nameEdit->setText(detail.GetName());
    m_nameEdit->setCursorPosition(0);

    m_pieceInfoWidget->Show(detail, tool->getData());
    m_blockConfigWidget->Show(detail, tool->getData());
    m_annotationsConfigWidget->Show(detail, tool->getData());
    m_thumbnailConfigWidget->Show(detail.GetThumbnailData(), tool->getData(), tool->getId());

    // Alignment: both rows, the forced point, and the enabled/disabled states that follow from it.
    ApplyAlignPointControls(detail, tool->getData());

    m_forceFlippingCheck->setChecked(detail.IsForceFlipping());
    m_symmetricalCopyCheck->setChecked(detail.IsSymmetricalCopy());

    // ── Grainline tab ────────────────────────────────────────────
    m_grainlineWidget->Show(detail.GetGrainlineGeometry(),
                            [this, detail, tool]() -> std::optional<QPointF>
    {
        // detail captured by value: the provider outlives this lambda, so a
        // reference would dangle and crash on the next grainline activation.
        return GrainlineCenter(detail, tool->getData());
    });
    m_foldlineWidget->Show(detail, tool->getData(), tool->getId());
    m_saWidget->Show(detail);
    const bool hadInternalPathHighlight = m_internalPathWidget->HasActivePath();
    m_internalPathWidget->Show(detail, tool->getData());

    m_updating = false;

    // The internal path highlight only lives while its tab is on screen with an active row.
    // Switching piece, or re-selecting the piece (Show() rebuilds the list from scratch and
    // drops the row) → clear the canvas highlight.
    if (previousPieceId != m_pieceId || hadInternalPathHighlight)
    {
        emit ClearHighlight();
    }

    // After that global wipe, never before: the orange point of the General tab is lit last.
    RefreshAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::Clear()
{
    m_currentTool = nullptr;
    m_pieceId = 0;
    m_pendingDet = VPiece{};
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_selectedPlaceLabelIds.clear();
    m_pieceInfoWidget->Clear();
    m_grainlineWidget->Clear();
    m_foldlineWidget->Clear();
    m_saWidget->Clear();
    m_internalPathWidget->Clear();
    m_blockConfigWidget->Clear();
    m_annotationsConfigWidget->Clear();
    m_thumbnailConfigWidget->Clear();
    m_alignPointSelectState = AlignPointSelectState::Idle;
    m_showAlignPointState = true;

    // Nothing selected anymore — drop any canvas highlight.
    ClearAlignPointHighlight();
    emit ClearHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ClearInternalPathActivation()
{
    // The Seam Allowance tab / piece page lost the focus: drop the active row (dot back to
    // circle) and turn the canvas highlight off.
    m_internalPathWidget->Deactivate();
    emit ClearHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::RefreshPieceAfterParse()
{
    // A full parse (undo/redo of a delete…) destroyed every tool and rebuilt the container. If the
    // piece page is still current and we know which piece it showed, resolve the recreated tool by
    // id and repopulate — otherwise the internal path list keeps its pre-parse state (an internal
    // path restored by undo never reappears).
    if (m_pieceId == 0 || m_currentTool)
    {
        return; // nothing to refresh, or the tool survived (page already up to date)
    }
    if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(m_pieceId)); tool != nullptr)
    {
        Show(tool);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ShowGrainlineTab()
{
    m_tabWidget->setCurrentIndex(1);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ShowBlocksTab()
{
    m_tabWidget->setCurrentIndex(3);
    m_infoSubTabs->setCurrentIndex(1);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ShowAnnotationsTab()
{
    m_tabWidget->setCurrentIndex(3);
    m_infoSubTabs->setCurrentIndex(2);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ShowThumbnailTab()
{
    m_tabWidget->setCurrentIndex(3);
    m_infoSubTabs->setCurrentIndex(3);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnInternalPathActivated(quint32 pathId)
{
    // One highlight at a time: drop any previous one, then light the activated path.
    emit ClearHighlight();
    // pathId == 0 is the toggle-off sentinel (re-click on the active row) — clear only.
    if (pathId != 0 && m_currentTool)
    {
        emit InternalPathHighlight(m_currentTool->getId(), pathId, true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnEditInternalPath(quint32 pathId)
{
    if (!m_currentTool)
    {
        return;
    }
    const auto *data = m_currentTool->getData();
    if (!data->ContainsPiecePath(pathId))
    {
        return;
    }

    const quint32 pieceId = m_currentTool->getId();
    const VPiecePath currentPath = data->GetPiecePath(pathId);
    auto *doc = VAbstractValApplication::VApp()->getCurrentDocument();

    // Dedicated attribute-edit dialog (DialogPiecePathOptions), built directly on the QDialog like
    // the "Internal mark" dialog: no Piece combo, no node list, no tab container → no "fond".
    auto *dialog = new DialogPiecePathOptions(data, pathId, this);
    dialog->SetPath(currentPath); // pre-fills every attribute field, keeps the path geometry

    // Highlight the edited path on the canvas while the dialog is open.
    emit InternalPathHighlight(pieceId, pathId, true);
    const int result = dialog->exec();
    emit InternalPathHighlight(pieceId, pathId, false);

    if (result == QDialog::Accepted)
    {
        const VPiecePath newPath = dialog->GetPath();
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePiecePathOptions(pieceId, currentPath, newPath, doc, pathId));
        m_pendingDet = data->GetPiece(pieceId); // resync after the command's redo
    }
    dialog->deleteLater();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::DeleteInternalPath(quint32 pathId)
{
    if (!m_currentTool)
    {
        return;
    }
    const auto *data = m_currentTool->getData();
    if (!data->ContainsPiecePath(pathId))
    {
        return;
    }

    const quint32 pieceId = m_currentTool->getId();
    const QString name = data->GetPiecePath(pathId).GetName();

    if (QMessageBox::question(this, tr("Delete internal path"),
                              tr("Delete internal path \"%1\"?").arg(name),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    const VPiece oldDet = data->GetPiece(pieceId);
    VPiece newDet = oldDet;
    newDet.GetInternalPaths().removeAll(pathId); // detach the path from the piece's <iPaths>

    auto *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    QUndoStack *stack = VAbstractApplication::VApp()->getUndoStack();

    // One undo step: detach the path from the piece, then really delete the path tool and its
    // <path> XML element (full parse). This is a true deletion — nothing stays masked/orphaned.
    stack->beginMacro(tr("Delete internal path"));
    stack->push(new SavePieceOptions(oldDet, newDet, doc, pieceId));
    auto *delTool = new DelTool(doc, pathId);
    connect(delTool, &DelTool::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing);
    stack->push(delTool);
    stack->endMacro();

    emit ClearHighlight();

    if (m_currentTool)
    {
        m_pendingDet = m_currentTool->getData()->GetPiece(pieceId);
    }
    else
    {
        m_pendingDet = VPiece{};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CommitPending()
{
    if (!m_currentTool || m_updating)
    {
        return;
    }

    m_updating = true;

    const auto *data = m_currentTool->getData();
    const VPiece oldDet = data->GetPiece(m_currentTool->getId());

    // A child moved from the dock (the grainline preset, a block or an annotation edited in the
    // property browser) is in m_pendingDet: record its anchor here, so that a later change of
    // size keeps it where it was put. The piece's own fields are unaffected.
    OChildAnchor::SyncFollowedChildren(oldDet, m_pendingDet, m_currentTool->ChildAnchorRefRect());

    // **A group adopts the fold line its member has just gained**, when there is nothing to choose:
    // no other member has one. The group never defines a fold line — it uses one of its members' —
    // so the first one is also the only one. Saving the piece and choosing the carrier is one
    // action, hence one undo step.
    const quint32 pieceId = m_currentTool->getId();
    const quint32 groupId = data->GetPieceGroupId(pieceId);
    OPieceGroup groupOld;
    bool adoptFoldLine = false;
    if (groupId != NULL_ID && m_pendingDet.GetFoldLineType() != FoldLineType::None)
    {
        const auto *groups = data->DataPieceGroups();
        if (groups != nullptr && groups->contains(groupId))
        {
            groupOld = groups->value(groupId);
            adoptFoldLine = groupOld.m_foldLineMemberId != pieceId;
            for (const quint32 memberId : groupOld.pieceIds)
            {
                if (!adoptFoldLine || memberId == pieceId)
                {
                    continue;
                }
                try
                {
                    if (data->GetPiece(memberId).GetFoldLineType() != FoldLineType::None)
                    {
                        adoptFoldLine = false; // another member has one: the choice is the user's
                    }
                }
                catch (const VExceptionBadId &)
                {
                    continue;
                }
            }
        }
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    if (adoptFoldLine)
    {
        undoStack->beginMacro(tr("Set the fold line"));
    }
    undoStack->push(new SavePieceOptions(oldDet, m_pendingDet,
                                         VAbstractValApplication::VApp()->getCurrentDocument(), pieceId));
    if (adoptFoldLine)
    {
        OPieceGroup groupNew = groupOld;
        groupNew.m_foldLineMemberId = pieceId;
        // The command writes the group back through the container, which the tool hands over as a
        // read-only view; the group it owns is exactly what this command is for.
        undoStack->push(new SavePieceGroupOptions(groupOld, groupNew,
                                                  VAbstractValApplication::VApp()->getCurrentDocument(),
                                                  const_cast<VContainer *>(data), groupId));
        undoStack->endMacro();
    }

    // Resync pending state with the container after the undo.
    m_pendingDet = data->GetPiece(m_currentTool->getId());

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingPassmarkPoints() const -> bool
{
    return m_passmarkSelectState != PassmarkSelectState::Idle;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnAddPassmarks()
{
    if (!m_currentTool)
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Adding)
    {
        CancelPassmarkSelection();
        return;
    }

    m_passmarkSelectState = PassmarkSelectState::Adding;
    m_selectedPassmarkNodeIds.clear();
    m_selectedPlaceLabelIds.clear();
    m_addPassmarksBtn->setText(tr("Cancel"));

    // Highlight existing passmarks in orange.
    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.GetTypeTool() == Tool::NodePoint && node.IsPassmark() && not node.IsExcluded())
        {
            emit HighlightNode(node.GetId(), true, QColor(255, 165, 0)); // orange
        }
    }

    emit ShowStatusTip(tr("Click points to add passmarks - Enter to confirm, Esc to cancel"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnRemovePassmarks()
{
    if (!m_currentTool)
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Removing)
    {
        CancelPassmarkSelection();
        return;
    }

    m_passmarkSelectState = PassmarkSelectState::Removing;
    m_selectedPassmarkNodeIds.clear();
    m_selectedPlaceLabelIds.clear();
    m_removePassmarksBtn->setText(tr("Cancel"));

    // Highlight existing passmarks in orange.
    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.GetTypeTool() == Tool::NodePoint && node.IsPassmark() && not node.IsExcluded())
        {
            emit HighlightNode(node.GetId(), true, QColor(255, 165, 0)); // orange
        }
    }

    // The internal marks of the piece are removable in the same gesture — highlight them too.
    for (quint32 labelId : piece.GetPlaceLabels())
    {
        emit HighlightNode(labelId, true, QColor(255, 165, 0)); // orange
    }

    emit ShowStatusTip(tr("Click passmark points and internal marks to remove - Enter to confirm, Esc to cancel"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelPassmarkSelection()
{
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_selectedPlaceLabelIds.clear();
    m_addPassmarksBtn->setText(tr("+ Add passmarks"));
    m_removePassmarksBtn->setText(tr("- Remove marks"));
    emit ClearHighlight();
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnPlaceLabelSelectedForPassmark(quint32 labelId)
{
    if (m_passmarkSelectState != PassmarkSelectState::Removing || !m_currentTool)
    {
        return;
    }

    // In Remove mode only the marks this piece actually owns may be selected.
    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    if (not piece.GetPlaceLabels().contains(labelId))
    {
        return;
    }

    if (m_selectedPlaceLabelIds.contains(labelId))
    {
        m_selectedPlaceLabelIds.remove(labelId);
        emit HighlightTemporaryNode(labelId, false);
    }
    else
    {
        m_selectedPlaceLabelIds.insert(labelId);
        emit HighlightTemporaryNode(labelId, true);
    }

    const auto count = m_selectedPassmarkNodeIds.size() + m_selectedPlaceLabelIds.size();
    emit ShowStatusTip(tr("%1 mark(s) selected - Enter to confirm, Esc to cancel").arg(count));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingFoldLinePoints() const -> bool
{
    return m_foldlineWidget->IsSelectingPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForFoldLine(quint32 nodeId)
{
    m_foldlineWidget->OnNodeSelected(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelFoldLineSelection()
{
    m_foldlineWidget->CancelSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingSAPoints() const -> bool
{
    return m_saWidget->IsSelectingPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForSA(quint32 nodeId)
{
    m_saWidget->OnNodeSelected(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeHoveredForSA(quint32 nodeId)
{
    m_saWidget->OnNodeHovered(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelSASelection()
{
    m_saWidget->CancelSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingAlignPoint() const -> bool
{
    return m_alignPointSelectState == AlignPointSelectState::Selecting;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelAlignPointSelection()
{
    if (m_alignPointSelectState == AlignPointSelectState::Idle)
    {
        return;
    }
    m_alignPointSelectState = AlignPointSelectState::Idle;
    m_alignPointBtn->setText(tr("Align on a point"));
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsEditingThumbnail() const -> bool
{
    return m_currentTool != nullptr && m_currentTool->IsThumbnailEditing();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelThumbnailEdit()
{
    if (m_currentTool != nullptr)
    {
        m_currentTool->CancelThumbnailEdit();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnAlignPointClicked()
{
    if (m_updating || !m_currentTool)
    {
        return;
    }
    if (m_alignPointSelectState == AlignPointSelectState::Selecting)
    {
        CancelAlignPointSelection();
        return;
    }

    // Passmarks are picked on this same tab: only one node-picking workflow may be armed at a time.
    CancelPassmarkSelection();

    m_alignPointSelectState = AlignPointSelectState::Selecting;
    m_alignPointBtn->setText(tr("Cancel"));
    emit ShowStatusTip(tr("Click the point to align on, on the piece outline (Esc to cancel)"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForAlignPoint(quint32 nodeId)
{
    if (m_alignPointSelectState != AlignPointSelectState::Selecting || m_updating || !m_currentTool)
    {
        return;
    }

    // The point must belong to the main path: that outline is what the alignment anchors on. A
    // click that landed on another piece of the canvas is not worth dropping the workflow over.
    if (m_pendingDet.GetPath().indexOfNode(nodeId) == -1)
    {
        emit ShowStatusTip(tr("This point is not on the piece outline - click a point of the piece"));
        return;
    }

    CancelAlignPointSelection();
    ModifyPiece([nodeId](VPiece &det) { det.SetGradedAlignPointId(nodeId); });

    // A freshly chosen point is shown straight away: the checkbox is on by default.
    m_showAlignPointState = true;
    ApplyAlignPointControls(m_pendingDet, m_currentTool->getData());
    RefreshAlignPointHighlight();

    // The piece stays the active object and the General tab the active tab: picking a point is not
    // selecting a node. The browser brings this page back once the click has finished travelling.
    emit PiecePageRequested();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::RestoreGeneralTab()
{
    m_tabWidget->setCurrentIndex(kGeneralTabIndex);
    RefreshAlignPointHighlight(); // the owning tab is on screen again: the point comes back on
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnRemoveAlignPoint()
{
    if (m_updating || !m_currentTool)
    {
        return;
    }
    CancelAlignPointSelection();
    ModifyPiece([](VPiece &det) { det.SetGradedAlignPointId(NULL_ID); });
    ApplyAlignPointControls(m_pendingDet, m_currentTool->getData());
    RefreshAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnShowAlignPointToggled(bool checked)
{
    if (m_updating)
    {
        return;
    }
    m_showAlignPointState = checked;
    RefreshAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ClearAlignPointHighlight()
{
    if (m_alignPointHighlightId == NULL_ID)
    {
        return;
    }
    emit HighlightNode(m_alignPointHighlightId, false);
    m_alignPointHighlightId = NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::RefreshAlignPointHighlight()
{
    // The point is owned by the General tab: it stays lit only while that tab is on screen, and
    // while its checkbox is on.
    const quint32 nodeId = m_pendingDet.GetGradedAlignPointId();
    const bool lit = nodeId != NULL_ID && m_showAlignPointCheck->isChecked()
                     && m_tabWidget->currentIndex() == kGeneralTabIndex;

    if (m_alignPointHighlightId != NULL_ID && (!lit || m_alignPointHighlightId != nodeId))
    {
        emit HighlightNode(m_alignPointHighlightId, false);
        m_alignPointHighlightId = NULL_ID;
    }
    if (lit)
    {
        emit HighlightNode(nodeId, true);
        m_alignPointHighlightId = nodeId;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsMemberOfGroup(const VContainer *data) const -> bool
{
    if (data == nullptr || m_pieceId == NULL_ID)
    {
        return false;
    }
    const quint32 groupId = data->GetPieceGroupId(m_pieceId);
    const auto *groups = data->DataPieceGroups();
    if (groupId == NULL_ID || groups == nullptr)
    {
        return false;
    }
    const auto found = groups->constFind(groupId);
    // The same threshold the alignment itself uses: below two members, a group is not a block.
    return found != groups->cend() && found.value().pieceIds.size() >= 2;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::SetAlignRowsEnabled(bool horizontal, bool vertical)
{
    // Qt refuses to uncheck the checked button of an exclusive group: release the exclusivity,
    // uncheck, then restore it. Without this, a disabled row would keep a stale checked button.
    const auto applyRow = [](QButtonGroup *group, QPushButton *const *btns, bool enabled)
    {
        if (!enabled)
        {
            group->setExclusive(false);
            for (int i = 0; i < 3; ++i)
            {
                btns[i]->setChecked(false);
            }
            group->setExclusive(true);
        }
        for (int i = 0; i < 3; ++i)
        {
            btns[i]->setEnabled(enabled);
        }
    };
    applyRow(m_alignHGroup, m_alignHBtns, horizontal);
    applyRow(m_alignVGroup, m_alignVBtns, vertical);
    m_alignHLabel->setEnabled(horizontal);
    m_alignVLabel->setEnabled(vertical);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ApplyAlignPointControls(const VPiece &det, const VContainer *data)
{
    const bool member = IsMemberOfGroup(data);
    const bool hasPoint = det.GetGradedAlignPointId() != NULL_ID;

    m_alignPointBtn->setText(m_alignPointSelectState == AlignPointSelectState::Selecting
                                 ? tr("Cancel")
                                 : tr("Align on a point"));
    m_alignPointBtn->setEnabled(!member);
    m_removeAlignPointBtn->setVisible(hasPoint);
    m_removeAlignPointBtn->setEnabled(!member);
    m_showAlignPointCheck->setEnabled(hasPoint && !member);
    m_showAlignPointCheck->setChecked(hasPoint && m_showAlignPointState);

    // A member piece takes its whole alignment from its group: both rows go inert. A forced point
    // takes the horizontal row over — the vertical one keeps working.
    SetAlignRowsEnabled(!member && !hasPoint, !member);

    int h = det.GetGradedAlignH();
    if (h == 0)
    {
        h = 2;
    }
    int v = det.GetGradedAlignV();
    if (v == 0)
    {
        v = 2;
    }
    if (!member && !hasPoint)
    {
        if (auto *hBtn = m_alignHGroup->button(h))
        {
            hBtn->setChecked(true);
        }
    }
    if (!member)
    {
        if (auto *vBtn = m_alignVGroup->button(v))
        {
            vBtn->setChecked(true);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForPassmark(quint32 nodeId)
{
    if (m_passmarkSelectState == PassmarkSelectState::Idle || !m_currentTool)
    {
        return;
    }

    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    const int idx = path.indexOfNode(nodeId);
    if (idx == -1)
    {
        return;
    }

    const VPieceNode &node = path.at(idx);
    if (node.GetTypeTool() != Tool::NodePoint || node.IsExcluded())
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Removing && not node.IsPassmark())
    {
        return; // in Remove mode, only existing passmark points may be selected
    }

    if (m_selectedPassmarkNodeIds.contains(nodeId))
    {
        m_selectedPassmarkNodeIds.remove(nodeId);
        emit HighlightTemporaryNode(nodeId, false);
    }
    else
    {
        m_selectedPassmarkNodeIds.insert(nodeId);
        emit HighlightTemporaryNode(nodeId, true);
    }

    const auto count = m_selectedPassmarkNodeIds.size() + m_selectedPlaceLabelIds.size();
    emit ShowStatusTip(tr("%1 mark(s) selected - Enter to confirm, Esc to cancel").arg(count));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::FinishPassmarkSelection()
{
    if (m_passmarkSelectState == PassmarkSelectState::Idle || !m_currentTool)
    {
        return;
    }

    if (m_selectedPassmarkNodeIds.isEmpty() && m_selectedPlaceLabelIds.isEmpty())
    {
        CancelPassmarkSelection();
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Adding)
    {
        // Open modal to choose passmark type.
        auto *dialog = new QDialog(this);
        dialog->setWindowTitle(tr("Choose Passmark Type"));
        auto *dlgLayout = new QVBoxLayout(dialog);
        auto *combo = new QComboBox();
        const struct
        {
            PassmarkLineType type;
            const char *text;
        } items[] = {
            // ⚠️ **Les littéraux DOIVENT être marqués.** `lupdate` n'extrait que les littéraux : une
            // chaîne atteinte par variable — ici `tr(item.text)` — n'est vue par personne. Ces neuf
            // libellés n'existaient donc dans **aucun** `.ts`, dans aucune langue, et s'affichaient
            // en anglais quelles que soient les préférences. C'est un défaut d'extraction, pas de
            // retraduction. Même marquage que la table de `osymbolshapes.cpp`.
            {PassmarkLineType::OneLine,       QT_TRANSLATE_NOOP("ODetailsPieceWidget", "One line")},
            {PassmarkLineType::TwoLines,      QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Two lines")},
            {PassmarkLineType::ThreeLines,    QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Three lines")},
            {PassmarkLineType::TMark,         QT_TRANSLATE_NOOP("ODetailsPieceWidget", "T mark")},
            {PassmarkLineType::ExternalVMark, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "External V mark")},
            {PassmarkLineType::InternalVMark, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Internal V mark")},
            {PassmarkLineType::UMark,         QT_TRANSLATE_NOOP("ODetailsPieceWidget", "U mark")},
            {PassmarkLineType::BoxMark,       QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Box mark")},
            {PassmarkLineType::CheckMark,     QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Check mark")},
        };
        int itemIndex = 0;
        for (const auto &item : items)
        {
            combo->addItem(QString(), static_cast<int>(item.type));
            OTranslation::SetItemText(this, combo, itemIndex, item.text);
            ++itemIndex;
        }
        dlgLayout->addWidget(combo);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
        dlgLayout->addWidget(buttons);

        if (dialog->exec() != QDialog::Accepted)
        {
            CancelPassmarkSelection();
            dialog->deleteLater();
            return;
        }
        const auto lineType = static_cast<PassmarkLineType>(combo->currentData().toInt());
        dialog->deleteLater();

        VPiecePath &pendingPath = m_pendingDet.GetPath();
        for (int i = 0; i < pendingPath.CountNodes(); ++i)
        {
            VPieceNode &node = pendingPath[i];
            if (m_selectedPassmarkNodeIds.contains(node.GetId()))
            {
                node.SetPassmark(true);
                node.SetPassmarkLineType(lineType);
            }
        }
    }
    else if (m_passmarkSelectState == PassmarkSelectState::Removing)
    {
        VPiecePath &pendingPath = m_pendingDet.GetPath();
        for (int i = 0; i < pendingPath.CountNodes(); ++i)
        {
            VPieceNode &node = pendingPath[i];
            if (m_selectedPassmarkNodeIds.contains(node.GetId()))
            {
                node.SetPassmark(false);
            }
        }
    }

    // Commit immediately — passmark operations are independent of Apply. When the gesture only
    // collected internal marks, the piece itself is unchanged: no empty undo step is pushed.
    if (m_passmarkSelectState == PassmarkSelectState::Adding || not m_selectedPassmarkNodeIds.isEmpty())
    {
        const auto *data = m_currentTool->getData();
        const VPiece oldDet = data->GetPiece(m_currentTool->getId());
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldDet, m_pendingDet,
                                 VAbstractValApplication::VApp()->getCurrentDocument(),
                                 m_currentTool->getId()));
        m_pendingDet = data->GetPiece(m_currentTool->getId());
    }

    // Internal marks are real objects, not node flags: they go through a real deletion (piece list +
    // <point> record), in the same gesture and the same undo step. That deletion tears the scene down,
    // so it comes last and leaves this method through VExceptionToolWasDeleted.
    if (m_passmarkSelectState == PassmarkSelectState::Removing && not m_selectedPlaceLabelIds.isEmpty())
    {
        const QVector<quint32> labelIds(m_selectedPlaceLabelIds.cbegin(), m_selectedPlaceLabelIds.cend());

        // The gesture is over: drop the mode (and its orange highlights) before the deletion. Leaving
        // the dock in Remove mode with a selection pointing at deleted marks served no purpose.
        CancelPassmarkSelection();

        try
        {
            m_currentTool->DeletePlaceLabels(labelIds);
        }
        catch (const VExceptionToolWasDeleted &e)
        {
            Q_UNUSED(e);
            return; // the full parse rebuilt the scene: nothing left to do here
        }
        return;
    }

    CancelPassmarkSelection();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnManageMainPath()
{
    if (!m_currentTool)
    {
        return;
    }

    const auto *data = m_currentTool->getData();
    const VPiece piece = data->GetPiece(m_currentTool->getId());

    auto *dialog = new DialogMainPath(data, this);
    dialog->SetNodes(piece.GetPath().GetNodes());

    if (dialog->exec() == QDialog::Accepted)
    {
        const QVector<VPieceNode> nodes = dialog->GetNodes();
        m_pendingDet.GetPath().SetNodes(nodes);

        const VPiece oldDet = data->GetPiece(m_currentTool->getId());
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldDet, m_pendingDet,
                                 VAbstractValApplication::VApp()->getCurrentDocument(),
                                 m_currentTool->getId()));
        m_pendingDet = data->GetPiece(m_currentTool->getId());
    }

    dialog->deleteLater();
}
