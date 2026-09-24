/************************************************************************
 **
 **  @file   oexportfavoriterowwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  One line of a favorites list: a tick box, the name, where it stands, and the
 **          trash that drops it.
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

#ifndef OEXPORTFAVORITEROWWIDGET_H
#define OEXPORTFAVORITEROWWIDGET_H

#include <QWidget>

#include "../core/oexportrunqueue.h"

class QCheckBox;
class QLabel;
class OExportRowIconButton;
class QPushButton;

/**
 * @brief One favorite of the batch: ticked to run it, named, and showing where it stands.
 *
 * Same shape as the "Working list" row — name, state, an icon button at the end — with a tick
 * box in front of it. The tick is what the whole sub-tab decides with: the batch generates every
 * favorite that is ticked, and every box starts ticked because a favorite was kept to be run.
 * Unticking one therefore means "not this time", not "I do not want this favorite any more".
 *
 * **The button at the end is a star, not a trash can.** A trash can in a list of favorites asks
 * a question the user cannot answer from the icon: does it take the favorite out of the store,
 * or the line out of this batch? It takes the favorite out — so it is drawn as the thing the row
 * IS, filled, and clicking it is the same gesture as the one that created it: the favorite stops
 * being one, and the line goes with it. A favorite outlives the generation it was starred from
 * (deleting that row in the Outputs dock leaves the recipe in the store), so this is the only way
 * to drop one whose dock row is gone.
 */
class OExportFavoriteRowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportFavoriteRowWidget(const QString &name, QWidget *parent = nullptr);
    ~OExportFavoriteRowWidget() override = default;

    /// @brief The favorite this row stands for. Also the key its state is written by.
    auto Name() const -> QString;

    /// @brief Write where this favorite's run stands.
    void SetState(OExportRunState state);

    auto CheckBox() const -> QCheckBox *;

    /// @brief The star: clicking it takes the favorite out.
    auto StarButton() const -> QPushButton *;

protected:
    void changeEvent(QEvent *event) override;

private:
    void Retranslate();

    QCheckBox *m_check{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLabel *m_stateLabel{nullptr};
    OExportRowIconButton *m_starButton{nullptr};
    OExportRunState m_state{OExportRunState::Pending};

    Q_DISABLE_COPY_MOVE(OExportFavoriteRowWidget) // NOLINT
};

#endif // OEXPORTFAVORITEROWWIDGET_H
