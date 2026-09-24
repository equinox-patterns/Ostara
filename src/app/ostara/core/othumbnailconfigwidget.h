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

#include "../vpatterndb/ipieceinfo.h"
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

    /** Callback that applies a piece-like mutation and commits it to the undo stack. */
    void SetModifyPending(std::function<void(std::function<void(IPieceInfo &)>)> callback);

    void Show(const IPieceInfo &detail, const VContainer *data, quint32 pieceId);
    void Clear();
    void RetranslateUI();

private:
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

    auto BuildData(const IPieceInfo &det) const -> OThumbnailData;
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

    std::function<void(std::function<void(IPieceInfo &)>)> m_modifyPending;

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
