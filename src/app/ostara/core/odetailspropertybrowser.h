/************************************************************************
 **
 **  @file   odetailspropertybrowser.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Property browser dock for Details mode (Modeling) — contextual
 **          stack (empty / point / piece / place label / group).
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

#ifndef ODETAILSPROPERTYBROWSER_H
#define ODETAILSPROPERTYBROWSER_H

#include <QColor>
#include <QObject>

class QDockWidget;
class QStackedWidget;
class ODetailsPieceWidget;
class VToolSeamAllowance;

class ODetailsPropertyBrowser : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsPropertyBrowser(QDockWidget *parent);
    ~ODetailsPropertyBrowser() override = default;

    void ShowEmpty();
    void ShowPieceOptions(VToolSeamAllowance *tool);
    void Clear();

    auto IsSelectingPassmarkPoints() const -> bool;
    void OnNodeSelectedForPassmark(quint32 nodeId);
    void FinishPassmarkSelection();
    void CancelPassmarkSelection();

    auto IsSelectingFoldLinePoints() const -> bool;
    void OnNodeSelectedForFoldLine(quint32 nodeId);
    void CancelFoldLineSelection();

    auto IsSelectingSAPoints() const -> bool;
    void OnNodeSelectedForSA(quint32 nodeId);
    void OnNodeHoveredForSA(quint32 nodeId);
    void CancelSASelection();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight, QColor color = Qt::red);
    void HighlightTemporaryNode(quint32 nodeId, bool selected);
    void ClearHighlight();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ODetailsPropertyBrowser) // NOLINT

    QStackedWidget *m_stack{nullptr};
    ODetailsPieceWidget *m_pieceWidget{nullptr};
};

#endif // ODETAILSPROPERTYBROWSER_H
