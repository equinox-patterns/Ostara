/************************************************************************
 **
 **  @file   odetailsgroupwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Group view of the Details options dock.
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

#include "odetailsgroupwidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QUndoStack>

#include "../vlayout/vfoldline.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepiecegroupoptions.h"
#include "../widgets/ogradedalignwidget.h"
#include "odetailsgrainlinewidget.h"
#include "opiececommonpoint.h"
#include "opiecegrouplayoutflags.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "../vtheme/vtheme.h"
#include "oannotationsconfigwidget.h"
#include "ogroupthumbnailsconfigwidget.h"
#include "oblockconfigwidget.h"
#include "odetailsfoldlinevalueswidget.h"
#include "ofoldlinetypes.h"
#include "opieceinfowidget.h"
#include "otranslation.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Toggle button of a member fold line: theme-aware caret-right (collapsed) / caret-down
// (expanded); while collapsed, the caret-right switches to the dark set on hover so it stays
// visible on the hover/accent background (dark theme already uses the dark set → no change).
class MemberFoldToggleButton : public QPushButton
{
public:
    explicit MemberFoldToggleButton(const QString &text, const QString &theme, QWidget *parent)
      : QPushButton(text, parent), m_theme(theme)
    {
        connect(this, &QPushButton::toggled, this, [this]() { UpdateIcon(); });
        // The dock survives a theme toggle: re-read the caret set instead of keeping the one captured
        // at construction. Enter/leave already refresh on their own, so only the idle state is at stake.
        connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]() {
            SetTheme(VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                               : QStringLiteral("light"));
        });
        UpdateIcon(); // initial state: collapsed, not hovered → theme caret-right
    }

    void SetTheme(const QString &theme)
    {
        m_theme = theme;
        UpdateIcon();
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        QPushButton::enterEvent(event);
        UpdateIcon();
    }
    void leaveEvent(QEvent *event) override
    {
        QPushButton::leaveEvent(event);
        UpdateIcon();
    }

private:
    void UpdateIcon()
    {
        if (isChecked())
        {
            setIcon(QIcon(QStringLiteral(":/icons/%1/caret-down_%1.svg").arg(m_theme)));
        }
        else if (underMouse())
        {
            setIcon(QIcon(QStringLiteral(":/icons/dark/caret-right_dark.svg")));
        }
        else
        {
            setIcon(QIcon(QStringLiteral(":/icons/%1/caret-right_%1.svg").arg(m_theme)));
        }
    }

    QString m_theme;
};

// Outer-tab indices of the group options (order of addTab), then the first sub-tab of the
// Information tab (Group / Blocks / Annotations). A group opens on the first of each.
constexpr int kGeneralTabIndex = 0;
constexpr int kGrainlineTabIndex = 1;
constexpr int kInformationTabIndex = 2;
constexpr int kGroupInfoSubTabIndex = 0;
constexpr int kBlocksSubTabIndex = 1;
constexpr int kAnnotationsSubTabIndex = 2;
constexpr int kThumbnailsSubTabIndex = 3;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsGroupWidget::ODetailsGroupWidget(QWidget *parent)
  : QWidget(parent)
{
    m_tabWidget = new QTabWidget(this);

    // ── General tab: QGroup Alignment only ───────────────────────────
    // Same widget as everywhere else (Dialog Piece step 2, PB Piece general tab):
    // Horizontal/Vertical labels + themed alignment icons. Page style matches the
    // PB Piece tab (SubtiliteCode §23): QFrame#detailsOptionsTabPage carries $DockBody.
    auto *generalPage = new QFrame(this);
    generalPage->setFrameStyle(QFrame::NoFrame);
    generalPage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *generalLayout = new QVBoxLayout(generalPage);
    m_alignWidget = new OGradedAlignWidget(generalPage);
    generalLayout->addWidget(m_alignWidget);

    // Forced alignment point: the group's common point — the one chosen when the pieces were
    // linked. Opt-in, and it overrides the horizontal row above; the vertical one keeps working.
    m_alignOnPointCheck = new QCheckBox(tr("Align on the common point"));
    m_alignWidget->AddExtraContent(m_alignOnPointCheck);
    m_showAlignPointCheck = new QCheckBox(tr("Show alignment point"));
    m_showAlignPointCheck->setChecked(true);
    m_alignWidget->AddExtraContent(m_showAlignPointCheck);
    connect(m_alignOnPointCheck, &QCheckBox::toggled, this, &ODetailsGroupWidget::OnAlignOnPointToggled);
    connect(m_showAlignPointCheck, &QCheckBox::toggled, this, &ODetailsGroupWidget::OnShowAlignPointToggled);

    // Layout flags. The group carries none of them — they belong to the pieces: ticking one here writes it into
    // every member, so the user neither unlinks the group nor ticks each member in the tree. Same labels and
    // texts as the piece browser.
    auto *flagsSeparator = new QFrame(generalPage);
    flagsSeparator->setFrameShape(QFrame::HLine);
    flagsSeparator->setFrameShadow(QFrame::Sunken);
    generalLayout->addWidget(flagsSeparator);

    m_forceFlippingCheck = new QCheckBox(tr("Force flipping"), generalPage);
    m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
    generalLayout->addWidget(m_forceFlippingCheck);

    m_symmetricalCopyCheck = new QCheckBox(tr("Symmetrical copy"), generalPage);
    m_symmetricalCopyCheck->setToolTip(tr("When the number of copies is greater than 1 and 'Force flipping' is selected, each even-numbered copy of the piece will be flipped vertically."));
    generalLayout->addWidget(m_symmetricalCopyCheck);

    connect(m_forceFlippingCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating)
        {
            return;
        }
        WriteLayoutFlags(checked, m_symmetricalCopyCheck->isChecked());
    });
    connect(m_symmetricalCopyCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating)
        {
            return;
        }
        WriteLayoutFlags(m_forceFlippingCheck->isChecked(), checked);
    });

    generalLayout->addStretch();

    connect(m_alignWidget, &OGradedAlignWidget::AlignHChanged, this,
            [this](int v)
    {
        if (m_updating)
        {
            return;
        }
        Commit([v](OPieceGroup &group) { group.alignH = v; });
    });
    connect(m_alignWidget, &OGradedAlignWidget::AlignVChanged, this,
            [this](int v)
    {
        if (m_updating)
        {
            return;
        }
        Commit([v](OPieceGroup &group) { group.alignV = v; });
    });

    // ⚠️ **Les titres d'onglets passent par le poseur.** Ils sont posés une fois ici et aucun
    // `setTabText` n'existait dans la classe : ils restaient donc dans la langue du premier
    // affichage. Le poseur n'exige aucune référence supplémentaire — le widget est déjà connu.
    const int generalTabIndex = m_tabWidget->addTab(generalPage, QString());
    OTranslation::SetTabText(this, m_tabWidget, generalTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "General"));

    // The common point is owned by the General tab: its orange highlight follows that tab.
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int) { RefreshAlignPointHighlight(); });

    // ── Grainline tab: common grainline + member foldlines ──────────
    // Same widget as the PB Piece (ODetailsGrainlineWidget) in a QFrame#detailsOptionsTabPage
    // (SubtiliteCode §23): activate grainline, length presets, rotation, simple/double.
    auto *grainlinePage = new QFrame(this);
    grainlinePage->setFrameStyle(QFrame::NoFrame);
    grainlinePage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *grainlineLayout = new QVBoxLayout(grainlinePage);

    m_grainlineWidget = new ODetailsGrainlineWidget(grainlinePage);
    grainlineLayout->addWidget(m_grainlineWidget);

    // **The group's fold line is one of its members'.** A fold line belongs to a piece — its points,
    // its type, its label are edited there, in the piece's Fold line tab. The group only says which
    // member's it uses, and the choice is all this section holds.
    m_foldLineSection = new QGroupBox(tr("Fold line"), grainlinePage);
    auto *foldLineLayout = new QVBoxLayout(m_foldLineSection);
    auto *foldLineRow = new QHBoxLayout();
    foldLineRow->addWidget(new QLabel(tr("Use the fold line of"), m_foldLineSection));
    m_foldLineCombo = MakeComboBox(m_foldLineSection);
    foldLineRow->addWidget(m_foldLineCombo, 1);
    foldLineLayout->addLayout(foldLineRow);
    m_foldLineHint = new QLabel(m_foldLineSection);
    m_foldLineHint->setWordWrap(true);
    foldLineLayout->addWidget(m_foldLineHint);
    grainlineLayout->addWidget(m_foldLineSection);
    connect(m_foldLineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsGroupWidget::OnFoldLineMemberChosen);
    grainlineLayout->addStretch();

    const int grainlineTabIndex = m_tabWidget->addTab(grainlinePage, QString());
    OTranslation::SetTabText(this, m_tabWidget, grainlineTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Grainline"));

    // ── Information tab: Group / Blocks / Annotations ────────────────
    // Same structure as the PB Piece's Information tab: the PAGE is the container
    // (QFrame#detailsOptionsTabPage carries $DockBody, margins 0), and each sub-tab
    // mirrors its PB Piece counterpart (SubtiliteCode §23).
    auto *infoPage = new QFrame(this);
    infoPage->setFrameStyle(QFrame::NoFrame);
    infoPage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *infoLayout = new QVBoxLayout(infoPage);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    auto *infoTabs = new QTabWidget(infoPage);
    infoTabs->setObjectName(QStringLiteral("informationTabWidget"));
    m_infoSubTabs = infoTabs; // kept to reset the sub-tab when the displayed group changes

    auto *groupTab = new QFrame(infoTabs);
    groupTab->setFrameStyle(QFrame::NoFrame);
    groupTab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *groupLayout = new QVBoxLayout(groupTab);
    groupLayout->setContentsMargins(8, 8, 8, 0);
    groupLayout->setSpacing(8);

    auto *form = new QFormLayout();
    m_nameEdit = new QLineEdit(groupTab);
    // Le libellé est créé par nous pour pouvoir être retraduit : `form->addRow(tr("Name"), …)`
    // confie le QLabel au QFormLayout, et plus personne ne peut le réécrire.
    auto *nameLabel = new QLabel(groupTab);
    OTranslation::SetText(this, nameLabel, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Name"));
    form->addRow(nameLabel, m_nameEdit);
    groupLayout->addLayout(form);

    m_groupInfoWidget = new OPieceInfoWidget(groupTab);
    groupLayout->addWidget(m_groupInfoWidget);
    groupLayout->addStretch(1);

    m_blockConfigWidget = new OBlockConfigWidget(infoTabs);
    m_annotationsConfigWidget = new OAnnotationsConfigWidget(infoTabs);
    m_thumbnailsConfigWidget = new OGroupThumbnailsConfigWidget(infoTabs);
    const int groupTabIndex = infoTabs->addTab(groupTab, QString());
    OTranslation::SetTabText(this, infoTabs, groupTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Group"));
    const int blocksTabIndex = infoTabs->addTab(m_blockConfigWidget, QString());
    OTranslation::SetTabText(this, infoTabs, blocksTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Blocks"));
    const int annotationsTabIndex = infoTabs->addTab(m_annotationsConfigWidget, QString());
    OTranslation::SetTabText(this, infoTabs, annotationsTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Annotations"));
    const int thumbnailTabIndex = infoTabs->addTab(m_thumbnailsConfigWidget, QString());
    OTranslation::SetTabText(this, infoTabs, thumbnailTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Thumbnail"));
    infoLayout->addWidget(infoTabs);
    const int informationTabIndex = m_tabWidget->addTab(infoPage, QString());
    OTranslation::SetTabText(this, m_tabWidget, informationTabIndex, QT_TRANSLATE_NOOP("ODetailsGroupWidget", "Information"));

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]()
    {
        if (m_updating)
        {
            return;
        }
        const QString name = m_nameEdit->text();
        Commit([name](OPieceGroup &group) { group.name = name; });
    });

    // Information sub-widgets commit through the group snapshot pipeline.
    const auto modifyGroup = [this](std::function<void(IPieceInfo &)> func)
    {
        Commit([func](OPieceGroup &group) { func(group); });
    };
    m_groupInfoWidget->SetModifyPending(modifyGroup);
    m_blockConfigWidget->SetModifyPending(modifyGroup);
    m_annotationsConfigWidget->SetModifyPending(modifyGroup);

    // The miniature is the group's own data — one block, drawn from all its members — so it commits
    // through the group snapshot pipeline, exactly like the group's blocks.
    m_thumbnailsConfigWidget->SetModifyGroup([this](std::function<void(OPieceGroup &)> func)
    {
        Commit(func);
    });

    // The common grainline commits through the group snapshot pipeline as well.
    m_grainlineWidget->SetModifyPending([this](std::function<void(VGrainlineData &)> func)
    {
        Commit([func](OPieceGroup &group) { func(group.m_glGrainline); });
    });

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_tabWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Show(quint32 groupId, VContainer *data, VAbstractPattern *doc)
{
    const quint32 previousGroupId = m_groupId;
    m_groupId = groupId;
    m_data = data;
    m_doc = doc;
    m_updating = true;

    // A group opens on its default tabs, whatever tab was left open on the previous group: this widget
    // instance is reused for every group and QTabWidget keeps its index. Refreshing the same group
    // leaves the tab the user is working in untouched.
    if (previousGroupId != m_groupId)
    {
        m_tabWidget->setCurrentIndex(kGeneralTabIndex);
        m_infoSubTabs->setCurrentIndex(kGroupInfoSubTabIndex);
        m_groupInfoWidget->ResetTabs(); // the cut tabs (Fabric / Interfacing / Lining) too
        m_showAlignPointState = true;   // the "show" checkbox opens checked
    }

    const auto *groups = data != nullptr ? data->DataPieceGroups() : nullptr;
    const OPieceGroup group = groups != nullptr ? groups->value(groupId) : OPieceGroup();

    // Name (Group sub-tab, same as the Piece sub-tab)
    if (m_nameEdit != nullptr)
    {
        m_nameEdit->setText(group.name);
    }

    // Alignment (OGradedAlignWidget — same widget as the piece dialog / piece browser).
    // The common point takes the horizontal row over: disable it first, then set the value back —
    // SetAlignHEnabled() unchecks, so a re-check after it would light up a dead row.
    if (m_alignWidget != nullptr)
    {
        const bool horizontalEnabled = !group.alignOnPoint;
        m_alignWidget->SetAlignHEnabled(horizontalEnabled);
        if (horizontalEnabled)
        {
            m_alignWidget->SetAlignH(group.alignH);
        }
        m_alignWidget->SetAlignV(group.alignV);
    }
    ApplyAlignPointControls(group);
    ShowLayoutFlags();

    // Common grainline editor (same widget as the PB Piece); the canvas holder
    // centers a freshly-activated grainline itself (no centerProvider here).
    m_grainlineWidget->Show(group.m_glGrainline);

    m_groupInfoWidget->Show(group, data);
    m_blockConfigWidget->Show(group, data);
    m_annotationsConfigWidget->Show(group, data);
    m_thumbnailsConfigWidget->Show(groupId, data);

    RefreshFoldLineChoices(group);

    m_updating = false;

    RefreshAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnAlignOnPointToggled(bool checked)
{
    if (m_updating)
    {
        return;
    }
    // Commit() re-Shows the group at the end, so the rows and the highlight resynchronise there.
    Commit([checked](OPieceGroup &group) { group.alignOnPoint = checked; });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnShowAlignPointToggled(bool checked)
{
    if (m_updating)
    {
        return;
    }
    m_showAlignPointState = checked;
    RefreshAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ClearAlignPointHighlight()
{
    if (m_alignPointHighlightId == NULL_ID)
    {
        return;
    }
    emit HighlightNode(m_alignPointHighlightId, false);
    m_alignPointHighlightId = NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::RefreshAlignPointHighlight()
{
    // The common point is lit through the reference member's own copy of it, and only while the
    // General tab — the one that owns it — is on screen with its checkbox on.
    const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    const OPieceGroup group = groups != nullptr ? groups->value(m_groupId) : OPieceGroup();

    quint32 nodeId = NULL_ID;
    if (group.alignPointSourceId != NULL_ID && group.alignOnPoint && m_showAlignPointCheck->isChecked()
        && m_tabWidget->currentIndex() == kGeneralTabIndex)
    {
        nodeId = ReferenceMemberNode(group);
    }

    if (m_alignPointHighlightId != NULL_ID && m_alignPointHighlightId != nodeId)
    {
        emit HighlightNode(m_alignPointHighlightId, false);
        m_alignPointHighlightId = NULL_ID;
    }
    if (nodeId != NULL_ID)
    {
        emit HighlightNode(nodeId, true);
        m_alignPointHighlightId = nodeId;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsGroupWidget::ReferenceMemberNode(const OPieceGroup &group) const -> quint32
{
    if (m_data == nullptr || group.alignPointSourceId == NULL_ID || group.alignPointMemberId == NULL_ID)
    {
        return NULL_ID;
    }
    const auto *pieces = m_data->DataPieces();
    if (pieces == nullptr || !pieces->contains(group.alignPointMemberId))
    {
        return NULL_ID;
    }
    return OPieceCommonPoint::PieceNodeWithSource(pieces->value(group.alignPointMemberId),
                                                  group.alignPointSourceId, m_data);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::RetranslateUI()
{
    // Le sélecteur d'alignement est construit par son propre constructeur : rien de lui n'est dans un
    // .ui, et `ui->retranslateUi` ne peut donc pas l'atteindre.
    m_alignWidget->RetranslateUI();

    m_alignOnPointCheck->setText(tr("Align on the common point"));
    m_showAlignPointCheck->setText(tr("Show alignment point"));

    m_forceFlippingCheck->setText(tr("Force flipping"));
    m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
    m_symmetricalCopyCheck->setText(tr("Symmetrical copy"));
    m_symmetricalCopyCheck->setToolTip(
        tr("When the number of copies is greater than 1 and 'Force flipping' is selected, each "
           "even-numbered copy of the piece will be flipped vertically."));

    m_foldLineSection->setTitle(tr("Fold line"));
    if (const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
        groups != nullptr && groups->contains(m_groupId))
    {
        RefreshFoldLineChoices(groups->value(m_groupId));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ApplyAlignPointControls(const OPieceGroup &group)
{
    const bool hasPoint = group.alignPointSourceId != NULL_ID;

    // Texts are set here rather than once at construction: the member rows are rebuilt on every
    // Show() too, so a language change is picked up the same way.
    m_alignOnPointCheck->setText(tr("Align on the common point"));
    m_showAlignPointCheck->setText(tr("Show alignment point"));

    // A group written before the common point was stored carries none: the choice cannot be offered.
    m_alignOnPointCheck->setEnabled(hasPoint);
    m_alignOnPointCheck->setChecked(hasPoint && group.alignOnPoint);
    m_showAlignPointCheck->setEnabled(hasPoint && group.alignOnPoint);
    m_showAlignPointCheck->setChecked(hasPoint && group.alignOnPoint && m_showAlignPointState);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ShowGrainlineTab()
{
    m_tabWidget->setCurrentIndex(kGrainlineTabIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ShowBlocksTab()
{
    m_tabWidget->setCurrentIndex(kInformationTabIndex);
    m_infoSubTabs->setCurrentIndex(kBlocksSubTabIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ShowAnnotationsTab()
{
    m_tabWidget->setCurrentIndex(kInformationTabIndex);
    m_infoSubTabs->setCurrentIndex(kAnnotationsSubTabIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ShowThumbnailTab()
{
    m_tabWidget->setCurrentIndex(kInformationTabIndex);
    m_infoSubTabs->setCurrentIndex(kThumbnailsSubTabIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Clear()
{
    m_groupId = NULL_ID;
    m_data = nullptr;
    m_doc = nullptr;
    m_showAlignPointState = true;
    ClearAlignPointHighlight();
    m_updating = true;
    if (m_nameEdit != nullptr)
    {
        m_nameEdit->clear();
    }
    m_grainlineWidget->Clear();
    m_groupInfoWidget->Clear();
    m_blockConfigWidget->Clear();
    m_annotationsConfigWidget->Clear();
    m_thumbnailsConfigWidget->Clear();

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Commit(std::function<void(OPieceGroup &)> func)
{
    if (m_updating || m_groupId == NULL_ID || m_data == nullptr || m_doc == nullptr)
    {
        return;
    }
    auto *groups = m_data->DataPieceGroups();
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return;
    }
    OPieceGroup oldGroup = groups->value(m_groupId);
    OPieceGroup newGroup = oldGroup;
    func(newGroup);
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SavePieceGroupOptions(oldGroup, newGroup, m_doc, m_data, m_groupId));

    // Re-sync the whole group view after the commit (the piece PB is re-shown by the
    // undo refresh; the group dock is not). Keeps the sub-tab lists — annotations,
    // blocks — in sync right after an add/edit/remove without re-clicking the group.
    Show(m_groupId, m_data, m_doc);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ModifyMemberPiece(quint32 pieceId, std::function<void(VPiece &)> func)
{
    if (m_data == nullptr || m_doc == nullptr || m_data->DataPieces() == nullptr ||
        not m_data->DataPieces()->contains(pieceId))
    {
        return;
    }
    VPiece oldDet = m_data->GetPiece(pieceId);
    VPiece newDet = oldDet;
    func(newDet);
    VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, m_doc, pieceId));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsGroupWidget::MemberPieceIds() const -> QVector<quint32>
{
    const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return {};
    }
    return groups->value(m_groupId).pieceIds;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ShowLayoutFlags()
{
    const QVector<quint32> ids = MemberPieceIds();

    m_updating = true;
    m_forceFlippingCheck->setChecked(OPieceGroupLayoutFlags::ForceFlipping(ids, m_data));
    m_symmetricalCopyCheck->setChecked(OPieceGroupLayoutFlags::SymmetricalCopy(ids, m_data));
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::WriteLayoutFlags(bool forceFlipping, bool symmetricalCopy)
{
    OPieceGroupLayoutFlags::Apply(MemberPieceIds(), forceFlipping, symmetricalCopy, m_data, m_doc);
    // One member may already have carried the value: the tick has to follow what the pieces now say, not the click.
    ShowLayoutFlags();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::RefreshFoldLineChoices(const OPieceGroup &group)
{
    if (m_foldLineCombo == nullptr || m_data == nullptr)
    {
        return;
    }

    // Filling the list is not a choice: the signals stay quiet, or every refresh would commit.
    const QSignalBlocker blocker(m_foldLineCombo);
    m_foldLineCombo->clear();
    m_foldLineCombo->addItem(tr("None"), QVariant::fromValue<quint32>(NULL_ID));

    int selectedIndex = 0;
    int candidates = 0;
    for (const quint32 pieceId : group.pieceIds)
    {
        VPiece member;
        try
        {
            member = m_data->GetPiece(pieceId);
        }
        catch (const VExceptionBadId &)
        {
            continue; // stale member id (hand-edited file) — skip it
        }

        // What makes a fold line: the type the piece draws. Without one there is nothing to use.
        if (member.GetFoldLineType() == FoldLineType::None)
        {
            continue;
        }
        ++candidates;
        m_foldLineCombo->addItem(member.GetName().isEmpty() ? tr("Unnamed") : member.GetName(),
                                 QVariant::fromValue<quint32>(pieceId));
        if (pieceId == group.m_foldLineMemberId)
        {
            selectedIndex = m_foldLineCombo->count() - 1;
        }
    }

    m_foldLineCombo->setEnabled(candidates > 0);
    // The stored carrier may have lost its fold line: then the group uses none, and the list says so
    // rather than naming a member with nothing to give.
    m_foldLineCombo->setCurrentIndex(selectedIndex);
    m_foldLineHint->setText(candidates > 0
                                ? tr("The group uses that member's fold line. It is edited on the "
                                     "piece, in its Grainline tab.")
                                : tr("No member has a fold line yet. Set one on a piece, in its "
                                     "Grainline tab."));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnFoldLineMemberChosen(int index)
{
    if (m_updating || m_groupId == NULL_ID || index < 0 || m_foldLineCombo == nullptr)
    {
        return;
    }
    const quint32 pieceId = m_foldLineCombo->itemData(index).value<quint32>();
    Commit([pieceId](OPieceGroup &group) { group.m_foldLineMemberId = pieceId; });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUI();
    }
    QWidget::changeEvent(event);
}
