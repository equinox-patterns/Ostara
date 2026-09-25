/************************************************************************
 **
 **  @file   othumbnailconfigwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Thumbnail sub-tab of the Information dock tab: per-piece
 **          activation of the piece miniature. The block itself is placed
 **          and resized on the canvas, so this tab only carries the switch
 **          and the explanation of what the miniature draws.
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

#ifndef OTHUMBNAILCONFIGWIDGET_H
#define OTHUMBNAILCONFIGWIDGET_H

#include <QVector>
#include <QWidget>

#include <functional>
#include <utility>

#include "../vpatterndb/othumbnaildata.h"

class QCheckBox;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QSpinBox;
class VContainer;

/**
 * @brief Thumbnail sub-tab of the Information dock tab.
 *
 * Mutates VPiece through the same snapshot + undo pipeline as the rest of the
 * dock (SetModifyPending / ModifyPiece). The miniature is an entity of its own:
 * activation lives here, geometry lives on the canvas block.
 */
class OThumbnailConfigWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OThumbnailConfigWidget(QWidget *parent = nullptr);
    ~OThumbnailConfigWidget() override = default;

    /** Callback that applies a miniature mutation to the live owner data and commits it to the undo
     *  stack. It hands over the **live** object — a piece for the piece page, the group's stored
     *  miniature of a member for the group page — never a copy made here: the geometry the canvas
     *  stored has to survive an activation change. */
    using ThumbnailModifier = std::function<void(std::function<void(OThumbnailData &)>)>;
    void SetModifyPending(ThumbnailModifier callback);

    /**
     * @brief Shows the miniature state of one owner.
     *
     * @param thumbnail The state edited — the piece's own on the piece page, the group's per-member
     *                  one on the group page. The only source of the switches and of the labels.
     * @param data      The container the seam-allowance widths are read from.
     * @param pieceId   The piece whose widths are listed.
     * @param ownerName When set, the labels name that owner ("Activate thumbnail for Manche") and the
     *                  inner group box reads as a plain section: the caller already brings the box
     *                  (the group page shows one per member), and a box in a box is two subjects.
     */
    void Show(const OThumbnailData &thumbnail, const VContainer *data, quint32 pieceId,
              const QString &ownerName = QString());
    void Clear();
    void RetranslateUI();

private:
    /// @brief The three texts that depend on whether the miniature has a named owner.
    void ApplyTexts();
    /// @brief Writes the widget's switches onto the data handed over by the owner's commit callback.
    void ApplySwitches(OThumbnailData &data) const;

    /**
     * @brief One seam-allowance width of the piece, with the words that print it and the count of
     * how many times it appears on the miniature.
     */
    struct Row
    {
        qreal mm{0.0};
        QLabel *label{nullptr};
        QSpinBox *spin{nullptr};
    };

    void Commit();
    /// @brief The distinct widths the piece is cut with — empty when it carries no seam allowance.
    static auto WidthsFor(quint32 pieceId, const VContainer *data) -> QVector<qreal>;
    static auto CountFor(const OThumbnailData &data, qreal mm) -> int;
    void BuildRows(const QVector<qreal> &widths, const OThumbnailData &data);
    void ClearRows();
    void SyncRows(const QVector<qreal> &widths, const OThumbnailData &data);
    void RefreshRowTexts();
    void CommitCount(qreal mm, int count);

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OThumbnailConfigWidget) // NOLINT

    ThumbnailModifier m_modifyPending;

    /// @brief The owner's name, when the miniature is shown for somebody else's piece (a group
    /// member). Empty on the piece page, where the piece is its own owner.
    QString m_ownerName;

    QCheckBox *m_activeCheck{nullptr};
    QGroupBox *m_infoGroup{nullptr};
    QLabel *m_hintLabel{nullptr};

    QLabel *m_seamTitle{nullptr};
    QLabel *m_seamHint{nullptr};
    /// @brief One row per margin. A grid, so the counts line up under the widest of the values —
    /// three rows of hand-made layouts would each hold their own column and none of them would agree.
    QGridLayout *m_rowsGrid{nullptr};
    QHBoxLayout *m_unitsLine{nullptr};
    QCheckBox *m_centimetersCheck{nullptr};
    QCheckBox *m_inchesCheck{nullptr};
    QVector<Row> m_rows;
    QVector<qreal> m_widths; // The last widths shown, so a mere count change rebuilds nothing.

    bool m_loading{false};
};

#endif // OTHUMBNAILCONFIGWIDGET_H
