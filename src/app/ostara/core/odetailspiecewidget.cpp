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

#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
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
#include "../vtools/undocommands/savepieceoptions.h"
#include "../vtools/undocommands/savepiecepathoptions.h"
#include "../vtheme/vtheme.h"

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

// Outer-tab index that hosts the "Internal outline" list — the Seam Allowance tab (order of addTab).
constexpr int kSeamAllowanceTabIndex = 2;
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
        m_foldlineWidget->Show(detail);
        m_saWidget->RefreshList(detail);
        if (m_internalPathWidget->RefreshList(detail, currentTool->getData()))
        {
            // The active path disappeared (undo/redo of a delete) — clear the canvas.
            emit ClearHighlight();
        }
        m_blockConfigWidget->Show(detail, currentTool->getData());
        m_annotationsConfigWidget->Show(detail, currentTool->getData());
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

        layout->addWidget(m_alignGroupBox);

        // Layout flags — flipping & grainline constraints. Same flags and texts as the piece
        // creation dialog step 2; this is where the choice made at creation can be changed.
        auto *flagsSeparator = new QFrame();
        flagsSeparator->setFrameShape(QFrame::HLine);
        flagsSeparator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(flagsSeparator);

        m_forbidFlippingCheck = new QCheckBox(tr("Forbid flipping"));
        m_forbidFlippingCheck->setToolTip(tr("Forbid piece be mirrored in a layout."));
        layout->addWidget(m_forbidFlippingCheck);

        m_forceFlippingCheck = new QCheckBox(tr("Force flipping"));
        m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
        layout->addWidget(m_forceFlippingCheck);

        m_symmetricalCopyCheck = new QCheckBox(tr("Symmetrical copy"));
        m_symmetricalCopyCheck->setToolTip(tr("When the number of copies is greater than 1 and either 'Force Flipping' or 'Forbid Flipping' is selected, each even-numbered copy of the piece will be flipped vertically. The 'Force Flipping' or 'Forbid Flipping' setting will also toggle for each copy, applying the opposite of the initially selected option for each subsequent copy."));
        layout->addWidget(m_symmetricalCopyCheck);

        m_followGrainlineCheck = new QCheckBox(tr("Follow grainline"));
        m_followGrainlineCheck->setToolTip(tr("Force piece to always follow grainline direction regardless of layout settings."));
        layout->addWidget(m_followGrainlineCheck);

        auto *flagsEndSeparator = new QFrame();
        flagsEndSeparator->setFrameShape(QFrame::HLine);
        flagsEndSeparator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(flagsEndSeparator);

        // Passmark multi-selection
        auto *passmarkRow = new QHBoxLayout();
        m_addPassmarksBtn = new QPushButton(tr("+ Add passmarks"));
        passmarkRow->addWidget(m_addPassmarksBtn);

        m_removePassmarksBtn = new QPushButton(tr("- Remove passmarks"));
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

        auto *internalOutlineLabel = new QLabel(tr("Internal outline"), tab);
        internalOutlineLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(internalOutlineLabel);

        m_internalPathWidget = new ODetailsInternalPathWidget(tab);
        layout->addWidget(m_internalPathWidget, 0, Qt::AlignTop); // hug the top, do not stretch

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("Seam Allowance"));
    }

    // ── Information tab — sub-tabs: Piece, Blocks, Annotations (Thumbnail later). ──
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
    m_manageMainPathBtn->setText(tr("Manage main path"));
    m_addPassmarksBtn->setText(m_passmarkSelectState == PassmarkSelectState::Adding ? tr("Cancel")
                                                                                     : tr("+ Add passmarks"));
    m_removePassmarksBtn->setText(m_passmarkSelectState == PassmarkSelectState::Removing ? tr("Cancel")
                                                                                         : tr("- Remove passmarks"));
    if (m_forbidFlippingCheck)
    {
        m_forbidFlippingCheck->setText(tr("Forbid flipping"));
    }
    if (m_forceFlippingCheck)
    {
        m_forceFlippingCheck->setText(tr("Force flipping"));
    }
    if (m_symmetricalCopyCheck)
    {
        m_symmetricalCopyCheck->setText(tr("Symmetrical copy"));
    }
    if (m_followGrainlineCheck)
    {
        m_followGrainlineCheck->setText(tr("Follow grainline"));
    }
    m_tabWidget->setTabText(0, tr("General"));
    m_tabWidget->setTabText(1, tr("Grainline"));
    m_tabWidget->setTabText(2, tr("Seam Allowance"));
    m_tabWidget->setTabText(3, tr("Information"));
    m_infoSubTabs->setTabText(0, tr("Piece"));
    m_infoSubTabs->setTabText(1, tr("Blocks"));
    m_infoSubTabs->setTabText(2, tr("Annotations"));

    // Sub-widgets (Grainline tab, Fold line, Seam Allowance, Internal path)
    m_grainlineWidget->RetranslateUI();
    m_foldlineWidget->RetranslateUI();
    m_saWidget->RetranslateUI();
    m_internalPathWidget->RetranslateUI();
    m_blockConfigWidget->RetranslateUI();
    m_annotationsConfigWidget->RetranslateUI();
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

    // The active internal outline row follows the tab focus: it lives in the Seam Allowance tab,
    // so leaving that tab (any other sub-tab) drops the row and its canvas highlight.
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index)
    {
        if (index != kSeamAllowanceTabIndex)
        {
            ClearInternalPathActivation();
        }
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

    // Graded alignment icons follow the current theme (Light/Dark toggle).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &ODetailsPieceWidget::RefreshAlignIcons);

    // ── Layout flags (flipping & grainline) ──────────────────────

    connect(m_forbidFlippingCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        if (checked && m_forceFlippingCheck->isChecked())
        {
            m_updating = true;
            m_forceFlippingCheck->setChecked(false);
            m_updating = false;
        }
        ModifyPiece([checked](VPiece &det) { det.SetForbidFlipping(checked); });
    });

    connect(m_forceFlippingCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        if (checked && m_forbidFlippingCheck->isChecked())
        {
            m_updating = true;
            m_forbidFlippingCheck->setChecked(false);
            m_updating = false;
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

    connect(m_followGrainlineCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        ModifyPiece([checked](VPiece &det) { det.SetFollowGrainline(checked); });
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

    m_updating = true;

    m_nameEdit->setText(detail.GetName());
    m_nameEdit->setCursorPosition(0);

    m_pieceInfoWidget->Show(detail, tool->getData());
    m_blockConfigWidget->Show(detail, tool->getData());
    m_annotationsConfigWidget->Show(detail, tool->getData());

    int h = detail.GetGradedAlignH();
    if (h == 0)
    {
        h = 2;
    }
    if (auto *hBtn = m_alignHGroup->button(h))
    {
        hBtn->setChecked(true);
    }

    int v = detail.GetGradedAlignV();
    if (v == 0)
    {
        v = 2;
    }
    if (auto *vBtn = m_alignVGroup->button(v))
    {
        vBtn->setChecked(true);
    }

    m_forbidFlippingCheck->setChecked(detail.IsForbidFlipping());
    m_forceFlippingCheck->setChecked(detail.IsForceFlipping());
    m_symmetricalCopyCheck->setChecked(detail.IsSymmetricalCopy());
    m_followGrainlineCheck->setChecked(detail.IsFollowGrainline());

    // ── Grainline tab ────────────────────────────────────────────
    m_grainlineWidget->Show(detail.GetGrainlineGeometry(),
                            [this, detail, tool]() -> std::optional<QPointF>
    {
        // detail captured by value: the provider outlives this lambda, so a
        // reference would dangle and crash on the next grainline activation.
        return GrainlineCenter(detail, tool->getData());
    });
    m_foldlineWidget->Show(detail);
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
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::Clear()
{
    m_currentTool = nullptr;
    m_pieceId = 0;
    m_pendingDet = VPiece{};
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_pieceInfoWidget->Clear();
    m_grainlineWidget->Clear();
    m_foldlineWidget->Clear();
    m_saWidget->Clear();
    m_internalPathWidget->Clear();
    m_blockConfigWidget->Clear();
    m_annotationsConfigWidget->Clear();

    // Nothing selected anymore — drop any canvas highlight.
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

    VAbstractApplication::VApp()->getUndoStack()->push(
        new SavePieceOptions(oldDet, m_pendingDet,
                             VAbstractValApplication::VApp()->getCurrentDocument(),
                             m_currentTool->getId()));

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

    emit ShowStatusTip(tr("Click points to add passmarks — Enter to confirm, Esc to cancel"));
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

    emit ShowStatusTip(tr("Click passmark points to remove — Enter to confirm, Esc to cancel"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelPassmarkSelection()
{
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_addPassmarksBtn->setText(tr("+ Add passmarks"));
    m_removePassmarksBtn->setText(tr("- Remove passmarks"));
    emit ClearHighlight();
    emit ShowStatusTip(QString());
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

    const auto count = m_selectedPassmarkNodeIds.size();
    emit ShowStatusTip(tr("%1 point(s) selected — Enter to confirm, Esc to cancel").arg(count));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::FinishPassmarkSelection()
{
    if (m_passmarkSelectState == PassmarkSelectState::Idle || !m_currentTool)
    {
        return;
    }

    if (m_selectedPassmarkNodeIds.isEmpty())
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
            {PassmarkLineType::OneLine,       "One line"},
            {PassmarkLineType::TwoLines,      "Two lines"},
            {PassmarkLineType::ThreeLines,    "Three lines"},
            {PassmarkLineType::TMark,         "T mark"},
            {PassmarkLineType::ExternalVMark, "External V mark"},
            {PassmarkLineType::InternalVMark, "Internal V mark"},
            {PassmarkLineType::UMark,         "U mark"},
            {PassmarkLineType::BoxMark,       "Box mark"},
            {PassmarkLineType::CheckMark,     "Check mark"},
        };
        for (const auto &item : items)
        {
            combo->addItem(tr(item.text), static_cast<int>(item.type));
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

    // Commit immediately — passmark operations are independent of Apply.
    {
        const auto *data = m_currentTool->getData();
        const VPiece oldDet = data->GetPiece(m_currentTool->getId());
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldDet, m_pendingDet,
                                 VAbstractValApplication::VApp()->getCurrentDocument(),
                                 m_currentTool->getId()));
        m_pendingDet = data->GetPiece(m_currentTool->getId());
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
