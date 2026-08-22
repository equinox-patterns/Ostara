/************************************************************************
 **
 **  @file   odetailsgrainlinewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Grainline editor widget for the Details mode options dock
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

#ifndef ODETAILSGRAINLINEWIDGET_H
#define ODETAILSGRAINLINEWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/vpiece.h"

class QButtonGroup;
class QCheckBox;
class QDoubleSpinBox;
class VContainer;

class ODetailsGrainlineWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsGrainlineWidget(QWidget *parent = nullptr);
    ~ODetailsGrainlineWidget() override = default;

    void Show(const VPiece &det, const VContainer *data);
    void Clear();

    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify);

private:
    void ConnectControls();
    void UpdatePresetsState(bool grainlineEnabled);

    QCheckBox *m_activeCheck{nullptr};
    QButtonGroup *m_lengthGroup{nullptr};
    QDoubleSpinBox *m_rotationSpin{nullptr};
    QButtonGroup *m_arrowGroup{nullptr};

    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;
    const VContainer *m_data{nullptr};
    bool m_updating{false};

    Q_DISABLE_COPY_MOVE(ODetailsGrainlineWidget) // NOLINT
};

#endif // ODETAILSGRAINLINEWIDGET_H
