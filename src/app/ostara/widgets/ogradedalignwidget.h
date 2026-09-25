/************************************************************************
 **
 **  @file   ogradedalignwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Graded alignment selector (horizontal + vertical, 3 buttons each).
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
class QGroupBox;
class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * @brief Horizontal/vertical graded alignment selector.
 *
 * Values follow VPiece::SetGradedAlignH/V: H 1=left, 2=center, 3=right;
 * V 1=top, 2=center, 3=bottom.
 */
class OGradedAlignWidget final : public QWidget
{
    Q_OBJECT

public:
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
    /// Disabling also unchecks it: the value it holds no longer drives anything (a forced alignment
    /// point, or a group member following its group), and a checked-but-dead row would read as if it
    /// still did. Re-enabling restores the enabled look only — the caller sets the value back from
    /// the model, so it must call SetAlignH() AFTER this.
    void SetAlignHEnabled(bool enabled);

    /// @brief Append a control at the bottom of the "Graded alignment" group box.
    ///
    /// Lets a dock put its own alignment options (the group's common point) inside the same frame
    /// as the rows they qualify. The piece dialog appends nothing and is left unchanged.
    void AddExtraContent(QWidget *widget);

    /// @brief Re-apply every text of the selector after a UI language change.
    ///
    /// The selector is built once, by its constructor, and neither the group box title nor the six
    /// button labels come from a .ui form: `ui->retranslateUi` cannot reach them.
    void RetranslateUI();

signals:
    void AlignHChanged(int value);
    void AlignVChanged(int value);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void RefreshAlignIcons();
    void UpdateButtonWidths();

    QGroupBox *m_alignGroupBox{nullptr};
    QVBoxLayout *m_alignLayout{nullptr};
    QButtonGroup *m_alignHGroup{nullptr};
    QButtonGroup *m_alignVGroup{nullptr};
    QLabel *m_alignHLabel{nullptr};
    QLabel *m_alignVLabel{nullptr};
    QPushButton *m_alignHBtns[3]{};
    QPushButton *m_alignVBtns[3]{};
};

#endif // OGRADEDALIGNWIDGET_H
