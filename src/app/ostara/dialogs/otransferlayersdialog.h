/************************************************************************
 **
 **  @file   otransferlayersdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Move layers of a group to another one — or out of every group,
 **          which is how a group is emptied.
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
#ifndef OTRANSFERLAYERSDIALOG_H
#define OTRANSFERLAYERSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVector>

class OLayerGroupChooser;
class QListWidget;
class QPushButton;

/// The transfer of a group's layers: WHICH ones leave, and WHERE they go. Both questions are asked once — a
/// transfer that left a layer behind by accident would have to be undone and redone.
class OTransferLayersDialog final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OTransferLayersDialog(QWidget *parent = nullptr);
    ~OTransferLayersDialog() override;

    /// The layers the group holds: every one of them is offered, all ticked. Moving the whole group is the usual
    /// answer, and taking one out of the move is a single click.
    void SetLayers(const QMap<quint32, QString> &layers);
    /// The groups the layers may join: the OTHERS, and nothing else — transferring into the group the layers are
    /// already in would move nothing. "No group" is among them: it is how a group is emptied.
    void SetTargets(const QMap<quint32, QString> &groups);

    auto SelectedLayers() const -> QVector<quint32>;
    /// NULL_ID for "no group" — the layers keep existing, they simply belong to no group any more.
    auto TargetGroup() const -> quint32;
    auto IsValid() const -> bool;

private:
    Q_DISABLE_COPY_MOVE(OTransferLayersDialog) // NOLINT

    QListWidget *m_layers{nullptr};
    OLayerGroupChooser *m_target{nullptr};
    QPushButton *m_acceptButton{nullptr};

    void RefreshState();
};

#endif // OTRANSFERLAYERSDIALOG_H
