/************************************************************************
 **
 **  @file   odetailsgroupwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Group view of the Details options dock: General (alignment),
 **          Grainline (common grainline + per-member foldlines) and
 **          Information (Group / Blocks / Annotations).
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

#ifndef ODETAILSGROUPWIDGET_H
#define ODETAILSGROUPWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

class QCheckBox;
class QButtonGroup;
class QComboBox;
class QFrame;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class OAnnotationsConfigWidget;
class ODetailsGrainlineWidget;
class OGradedAlignWidget;
class OGroupThumbnailsConfigWidget;
class OPieceInfoWidget;
class OBlockConfigWidget;
class VAbstractPattern;
class VContainer;

/**
 * @brief The ODetailsGroupWidget class — Options dock page for a selected group.
 *
 * All edits commit through the undo pipeline: group fields via
 * SavePieceGroupOptions, member foldlines via SavePieceOptions (the foldline
 * geometry lives on the member piece).
 */
class ODetailsGroupWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsGroupWidget(QWidget *parent = nullptr);
    /// @brief Retraduit le panneau au changement de langue.
    ///
    /// Doc Qt, « Prepare for Dynamic Language Changes » : `LanguageChange` parcourt tout l'arbre des
    /// widgets, et **chaque widget doit surcharger `changeEvent`** — la classe de base ne retraduit
    /// rien. Sans cette surcharge, ce panneau garde la langue de sa construction.
    void changeEvent(QEvent *event) override;

    ~ODetailsGroupWidget() override = default;

    void Clear();

    /// @brief Re-apply every text the widget wrote in code.
    ///
    /// The page has no .ui form, and neither has the graded alignment selector it embeds: nothing
    /// here is reachable by `ui->retranslateUi`, so the details property browser must call this.
    void RetranslateUI();

    /// @brief Switches to the tab that configures what was clicked on the canvas — the same routing
    /// a piece's children use (grainline, info blocks, annotations, miniature).
    void ShowGrainlineTab();
    void ShowBlocksTab();
    void ShowAnnotationsTab();
    void ShowThumbnailTab();
    void Show(quint32 groupId, VContainer *data, VAbstractPattern *doc);
    /** @brief True if this group is the one currently displayed. The canvas holder re-emits
     *  Clicked on the press that starts a drag, so the property browser skips a redundant
     *  re-population that would flicker the lists once. */
    auto IsShowing(quint32 groupId) const -> bool { return m_groupId == groupId; }

    /** @brief Drop the orange highlight on the group's common point, if any. */
    void ClearAlignPointHighlight();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight);

private:
    void Commit(std::function<void(OPieceGroup &)> func);
    void ModifyMemberPiece(quint32 pieceId, std::function<void(VPiece &)> func);
    // The layout flags belong to the members, not to the group: reading shows what the members say, writing
    // writes into all of them (one undo step). See OPieceGroupLayoutFlags.
    auto MemberPieceIds() const -> QVector<quint32>;
    void ShowLayoutFlags();
    void WriteLayoutFlags(bool forceFlipping, bool symmetricalCopy);
    /// The group's fold line is one of its members': this fills the list of members that have one
    /// and shows the chosen one. Nothing is created here — a fold line belongs to a piece.
    void RefreshFoldLineChoices(const OPieceGroup &group);
    void OnFoldLineMemberChosen(int index);

    QTabWidget *m_tabWidget{nullptr};
    QTabWidget *m_infoSubTabs{nullptr};
    OGradedAlignWidget *m_alignWidget{nullptr};

    // Forced alignment point: the group's common point, the one chosen when the pieces were linked.
    QCheckBox *m_alignOnPointCheck{nullptr};
    QCheckBox *m_showAlignPointCheck{nullptr};

    // Layout flags of the members. Checked when EVERY member carries the flag.
    QCheckBox *m_forceFlippingCheck{nullptr};
    QCheckBox *m_symmetricalCopyCheck{nullptr};
    bool m_showAlignPointState{true};          // UI state, per group: on by default
    quint32 m_alignPointHighlightId{NULL_ID};  // node currently painted orange, 0 when none

    void ApplyAlignPointControls(const OPieceGroup &group);
    void RefreshAlignPointHighlight();
    void OnAlignOnPointToggled(bool checked);
    void OnShowAlignPointToggled(bool checked);
    auto ReferenceMemberNode(const OPieceGroup &group) const -> quint32;

    // Information — Group sub-tab (same structure as the PB Piece's Piece sub-tab)
    QLineEdit *m_nameEdit{nullptr};

    // Common grainline editor (same widget as the piece)
    ODetailsGrainlineWidget *m_grainlineWidget{nullptr};

    // Information sub-tabs
    OPieceInfoWidget *m_groupInfoWidget{nullptr};
    OBlockConfigWidget *m_blockConfigWidget{nullptr};
    OAnnotationsConfigWidget *m_annotationsConfigWidget{nullptr};
    /// @brief One miniature editor per member: the group owns no miniature of its own, so this tab
    /// edits the members' pieces — through the same SavePieceOptions command a piece page uses.
    OGroupThumbnailsConfigWidget *m_thumbnailsConfigWidget{nullptr};

    // The fold line the group uses, chosen among its members
    QGroupBox *m_foldLineSection{nullptr};
    QComboBox *m_foldLineCombo{nullptr};
    QLabel *m_foldLineHint{nullptr};

    quint32 m_groupId{NULL_ID};
    VContainer *m_data{nullptr};
    VAbstractPattern *m_doc{nullptr};

    bool m_updating{false};

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ODetailsGroupWidget) // NOLINT
};

#endif // ODETAILSGROUPWIDGET_H
