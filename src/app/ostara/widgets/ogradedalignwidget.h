/************************************************************************
 **
 **  @file   ogradedalignwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Graded alignment selector (horizontal + vertical, 3 anchors and a forced point each).
 **          Used by the piece tool dialog (step 2); the Options dock carries an
 **          equivalent implementation.
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

#ifndef OGRADEDALIGNWIDGET_H
#define OGRADEDALIGNWIDGET_H

#include <QWidget>

class QButtonGroup;
class QCheckBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * @brief Horizontal/vertical graded alignment selector.
 *
 * Values follow VPiece::SetGradedAlignH/V: H 1=left, 2=center, 3=right;
 * V 1=top, 2=center, 3=bottom. Each row carries a fourth value, ValueOnPoint: the axis is then
 * carried by a point of the piece outline instead of by an anchor, and the row drops its anchors.
 */
class OGradedAlignWidget final : public QWidget
{
    Q_OBJECT

public:
    /// Fourth value of a row: the axis follows a point of the outline, not an anchor.
    static constexpr int ValueOnPoint = 4;

    /// @brief Retraduit le panneau au changement de langue.
    ///
    /// Doc Qt, « Prepare for Dynamic Language Changes » : `LanguageChange` parcourt tout l'arbre des
    /// widgets, et **chaque widget doit surcharger `changeEvent`** — la classe de base ne retraduit
    /// rien. Sans cette surcharge, ce panneau garde la langue de sa construction.
    void changeEvent(QEvent *event) override;

    explicit OGradedAlignWidget(QWidget *parent = nullptr);

    void SetAlignH(int value);
    auto GetAlignH() const -> int;
    void SetAlignV(int value);
    auto GetAlignV() const -> int;

    /// @brief Enable/disable the horizontal row.
    ///
    /// Disabling also unchecks it: the value it holds no longer drives anything (a group member
    /// following its group), and a checked-but-dead row would read as if it still did. Re-enabling
    /// restores the enabled look only — the caller sets the value back from the model, so it must
    /// call SetAlignH() AFTER this.
    void SetAlignHEnabled(bool enabled);

    /// @brief Vertical counterpart of SetAlignHEnabled.
    void SetAlignVEnabled(bool enabled);

    /// @brief Show the forced-point controls of both rows.
    ///
    /// The fourth button, its "Remove point" button and its "Show alignment point" checkbox only
    /// mean something where a point can be picked and stored. The piece creation dialog has neither
    /// a graded outline to pick on nor a size to align: it leaves them hidden, and the selector shows
    /// just the anchors it can honour.
    void SetAlignmentPointAvailable(bool available);

    /// @brief Reveal the "Remove point" button and the "Show alignment point" checkbox of one row.
    ///
    /// Called by the owner every time it re-reads its model: shown only while that axis really is
    /// carried by a point.
    void SetAlignPointActive(bool vertical, bool active);

    /// @brief Whether the point of that axis is to be lit on the canvas (the checkbox state).
    auto IsAlignPointShown(bool vertical) const -> bool;

    /// @brief Set the checkbox state without emitting (model re-read).
    void SetAlignPointShown(bool vertical, bool shown);

    /// @brief Re-apply every text of the selector after a UI language change.
    ///
    /// The selector is built once, by its constructor, and neither the group box title nor the six
    /// button labels come from a .ui form: `ui->retranslateUi` cannot reach them.
    void RetranslateUI();

signals:
    void AlignHChanged(int value); ///< 1/2/3, or ValueOnPoint when the row was just switched to a point
    void AlignVChanged(int value);
    void RemoveAlignPointRequested(bool vertical);
    void ShowAlignPointChanged(bool vertical, bool shown);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void RefreshAlignIcons();
    void UpdateButtonWidths();
    void BuildRow(bool vertical);

    QGroupBox *m_alignGroupBox{nullptr};
    QVBoxLayout *m_alignLayout{nullptr};
    QButtonGroup *m_alignHGroup{nullptr};
    QButtonGroup *m_alignVGroup{nullptr};
    QLabel *m_alignHLabel{nullptr};
    QLabel *m_alignVLabel{nullptr};
    QPushButton *m_alignHBtns[4]{};
    QPushButton *m_alignVBtns[4]{};
    QPushButton *m_removeHPointBtn{nullptr};
    QPushButton *m_removeVPointBtn{nullptr};
    QCheckBox *m_showHPointCheck{nullptr};
    QCheckBox *m_showVPointCheck{nullptr};
    bool m_alignmentPointAvailable{false};
};

#endif // OGRADEDALIGNWIDGET_H
