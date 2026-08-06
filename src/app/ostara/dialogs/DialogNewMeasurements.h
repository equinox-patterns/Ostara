/************************************************************************
 **
 **  @file   DialogNewMeasurements.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Dialog for creating a new measurement table (Solo/Multi + unit + template).
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

#ifndef DIALOGNEWMEASUREMENTS_H
#define DIALOGNEWMEASUREMENTS_H

#include <QDialog>

#include "vmisc/def.h"

class QButtonGroup;
class QComboBox;
class QLineEdit;
class QRadioButton;

class DialogNewMeasurements : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNewMeasurements(QWidget *parent = nullptr);
    ~DialogNewMeasurements() override = default;

    auto MeasurementsType() const -> ::MeasurementsType;
    auto Unit() const -> ::Unit;
    auto TemplatePath() const -> QString;
    auto UseTemplate() const -> bool;
    auto TableName() const -> QString;

private:
    QLineEdit *m_nameEdit{nullptr};
    QButtonGroup *m_typeGroup{nullptr};
    QRadioButton *m_multiRadio{nullptr};
    QRadioButton *m_soloRadio{nullptr};

    QButtonGroup *m_unitGroup{nullptr};
    QRadioButton *m_cmRadio{nullptr};
    QRadioButton *m_mmRadio{nullptr};
    QRadioButton *m_inchRadio{nullptr};

    QComboBox *m_templateCombo{nullptr};
    QStringList m_templatePaths;
};

#endif // DIALOGNEWMEASUREMENTS_H
