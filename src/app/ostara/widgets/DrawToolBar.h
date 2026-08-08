/************************************************************************
 **
 **  @file   DrawToolBar.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/08/2026
 **
 **  @brief  Responsive left dock toolbar for drawing tools.
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
#ifndef DRAWTOOLBAR_H
#define DRAWTOOLBAR_H

#include <QDockWidget>
#include <QHash>
#include <QVector>

#include "def.h"

class QAction;
class QToolButton;
class QVBoxLayout;

class DrawToolBar : public QDockWidget
{
    Q_OBJECT

public:
    explicit DrawToolBar(QWidget *parent = nullptr);
    ~DrawToolBar() override;

    void SetMode(Draw mode);
    void SetActions(const QHash<QString, QAction *> &actions);
    void ReloadIcons();
    void RestoreSavedWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto minimumSizeHint() const -> QSize override;

private:
    struct ToolItem
    {
        QString id;
        QString name;
        QAction *action;
        QToolButton *button;
    };

    struct ToolGroup
    {
        QString name;
        QVector<ToolItem> tools;
        int modeMask = 0; // bitmask of modes (kModeDraw | kModeDetail | kModeLayout)
    };

    void BuildLayout();
    int  ComputeColumns() const;
    int  ComputeMinWidth() const;
    void InitializeGroups();
    void UpdateBackground();

    int m_lastColCount = 0;
    int m_lastHeight = 0;
    bool m_building = false;
    bool m_settingMode = false;

    QWidget *m_root = nullptr;
    QVBoxLayout *m_rootLayout = nullptr;

    QHash<QString, QToolButton *> m_buttonMap;
    QVector<ToolGroup> m_groups;
    QList<QToolButton *> m_caretButtons;
    Draw m_currentMode = Draw::Calculation;
};

#endif // DRAWTOOLBAR_H
