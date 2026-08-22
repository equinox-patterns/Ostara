/************************************************************************
 **
 **  @file   preferencesappearancepage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22 August, 2026
 **
 **  @brief  Label appearance preferences page (Foldline text)
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

#ifndef PREFERENCESAPPEARANCEPAGE_H
#define PREFERENCESAPPEARANCEPAGE_H

#include <QWidget>

class QComboBox;
class QFontComboBox;
class QSpinBox;

class PreferencesAppearancePage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit PreferencesAppearancePage(QWidget *parent = nullptr);
    ~PreferencesAppearancePage() override = default;

    auto Apply() -> QStringList;

private:
    Q_DISABLE_COPY_MOVE(PreferencesAppearancePage) // NOLINT

    void InitFoldLineText();

    QFontComboBox *m_fontFamily{nullptr};
    QComboBox *m_weightCombo{nullptr};
    QSpinBox *m_sizeSpin{nullptr};
};

#endif // PREFERENCESAPPEARANCEPAGE_H
