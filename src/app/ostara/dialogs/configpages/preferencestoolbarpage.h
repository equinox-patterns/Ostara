/************************************************************************
 **
 **  @file   preferencestoolbarpage.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#ifndef PREFERENCETOOLBARPAGE_H
#define PREFERENCETOOLBARPAGE_H

#include "../../core/otoolbarlayout.h"

#include <QStringList>
#include <QVector>
#include <QWidget>

class QGroupBox;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;

/**
 * @brief The tool bar page — how the tools of the Draw mode are grouped.
 *
 * Left: the groups, with add / up / down / delete. Right: the tools of the selected group, with
 * up / down / delete. Bottom: every tool that belongs to no group, with a « + » that puts the
 * selected one into the selected group.
 *
 * **A tool in no group is a tool that is not shown.** That is the whole point of the page: one
 * the user never uses can leave the palette without leaving the application.
 *
 * Nothing is written before Apply(): the page edits a copy of the layout, so Cancel leaves the
 * palette and the settings exactly as they were.
 */
class PreferencesToolBarPage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit PreferencesToolBarPage(QWidget *parent = nullptr);
    ~PreferencesToolBarPage() override;

    auto Apply() -> QStringList;

protected:
    void changeEvent(QEvent *event) override;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(PreferencesToolBarPage) // NOLINT

    QVector<OToolBarLayout::Group> m_groups;
    QStringList m_hidden; // tools in no group, in catalogue order
    bool m_reloading{false};

    QGroupBox *m_groupBox{nullptr};
    QGroupBox *m_toolBox{nullptr};
    QGroupBox *m_hiddenBox{nullptr};

    QListWidget *m_groupList{nullptr};
    QListWidget *m_toolList{nullptr};
    QListWidget *m_hiddenList{nullptr};

    QToolButton *m_addGroup{nullptr};
    QToolButton *m_upGroup{nullptr};
    QToolButton *m_downGroup{nullptr};
    QToolButton *m_removeGroup{nullptr};

    QToolButton *m_upTool{nullptr};
    QToolButton *m_downTool{nullptr};
    QToolButton *m_removeTool{nullptr};

    QToolButton *m_assignTool{nullptr};
    QPushButton *m_defaults{nullptr};

    void BuildUi();
    void LoadLayout();
    void SetDefaultLayout();
    void ComputeHidden(const OToolBarLayout::Layout &layout);
    void ReloadLists();
    void ReloadTools();

    void AddGroup();
    void MoveGroup(int delta);
    void RemoveGroup();
    void RenameGroup(QListWidgetItem *item);

    void MoveTool(int delta);
    void RemoveTool();
    void AssignTool();
    void RestoreDefaults();

    auto CurrentGroup() const -> int;
    auto GroupTools(int group) -> QStringList &;

    void RetranslateUi();
    void UpdateButtons();
};

#endif // PREFERENCETOOLBARPAGE_H
