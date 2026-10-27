/************************************************************************
 **
 **  @file   preferencesgeneralpage.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef PREFERENCESGENERALPAGE_H
#define PREFERENCESGENERALPAGE_H

#include <QWidget>

namespace Ui
{
class PreferencesGeneralPage;
}

/**
 * @brief The first preferences page: language, units, theme and the application's own behaviour.
 *
 * It was one tab of the old « Configuration » page, beside Shortcuts; the two are now pages of their
 * own, so each can be reached in one click from the left column.
 */
class PreferencesGeneralPage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit PreferencesGeneralPage(QWidget *parent = nullptr);
    ~PreferencesGeneralPage() override;

    auto Apply() -> QStringList;

protected:
    void changeEvent(QEvent *event) override;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(PreferencesGeneralPage) // NOLINT
    Ui::PreferencesGeneralPage *ui;
    bool m_langChanged{false};
    bool m_unitChanged{false};

    void SetThemeModeComboBox();
    void InitUnits();
    void RetranslateUi();
};

#endif // PREFERENCESGENERALPAGE_H
